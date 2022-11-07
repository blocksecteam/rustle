use near_contract_standards::fungible_token::core::ext_ft_core;
use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::json_types::U128;
use near_sdk::serde::Serialize;
use near_sdk::{env, BorshStorageKey, Promise, PromiseResult};
use near_sdk::{near_bindgen, AccountId, Gas};

pub const TGAS: u64 = 1_000_000_000_000;
const GAS_FOR_FT_TRANSFER: Gas = Gas(10 * TGAS);
const GAS_FOR_CALLBACK: Gas = Gas(10 * TGAS);

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Orders,
}

#[derive(BorshDeserialize, BorshSerialize, Serialize, Clone)]
#[serde(crate = "near_sdk::serde")]
pub struct Order {
    token_id: AccountId,
    user: AccountId,
    amount: u128,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    orders: UnorderedMap<u16, Order>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            orders: UnorderedMap::new(StorageKey::Orders),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn process_redeem(&mut self, order_id: u16, amount: U128) -> Promise {
        let amount = amount.0;
        let mut order = self.orders.get(&order_id).unwrap();

        assert!(amount <= order.amount, "balance is insufficient");

        self.orders.remove(&order_id);
        order.amount -= amount;
        if order.amount > 0 {
            // only put `order` back when its `amount` is larger than 0
            self.orders.insert(&order_id, &order);
        }

        ext_ft_core::ext(order.token_id.clone())
            .with_attached_deposit(1)
            .with_static_gas(GAS_FOR_FT_TRANSFER)
            .ft_transfer(order.user.clone(), U128(amount), None)
            .then(
                Self::ext(env::current_account_id())
                    .with_static_gas(GAS_FOR_CALLBACK)
                    .callback_redeem(
                        order_id,
                        order.token_id.clone(),
                        order.user.clone(),
                        U128(order.amount + amount), // original amount
                    ),
            )
    }

    #[private]
    pub fn callback_redeem(
        &mut self,
        order_id: u16,
        token_id: AccountId,
        user: AccountId,
        amount: U128,
    ) {
        match env::promise_result(0) {
            PromiseResult::NotReady => unreachable!(),
            PromiseResult::Successful(_) => {}
            PromiseResult::Failed => {
                // this assertion can prevent deleted [Order] from getting recovered
                // this will happen when another [Order] seizes the `order_id`
                assert!(self.orders.get(&order_id).is_none());

                // recover [Order] state when ft_transfer fails
                self.orders.insert(
                    &order_id,
                    &Order {
                        token_id,
                        user,
                        amount: amount.into(),
                    },
                );
            }
        }
    }
}
