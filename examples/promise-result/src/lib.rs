use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::{env, ext_contract, near_bindgen, AccountId, Gas, Promise, PromiseResult};

pub const TGAS: u64 = 1_000_000_000_000;
const GAS_FOR_WITHDRAW_CALLBACK: Gas = Gas(20 * TGAS);

#[ext_contract(ext_self)]
pub trait SelfContract {
    fn callback_withdraw(&mut self, amount: U128);
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    depositor: AccountId,
    balance: u128,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            balance: 100,
            depositor: "depositor.near".parse().unwrap(),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn withdraw(&mut self, amount: U128) -> Promise {
        assert!(self.balance >= amount.into(), "insufficient balance");

        self.balance -= amount.0;

        Promise::new(self.depositor.clone())
            .transfer(amount.0)
            .then(
                ext_self::ext(env::current_account_id())
                    .with_static_gas(GAS_FOR_WITHDRAW_CALLBACK)
                    .callback_withdraw(amount),
            )
    }

    #[private]
    pub fn callback_withdraw(&mut self, amount: U128) {
        match env::promise_result(0) {
            PromiseResult::NotReady => unreachable!(),
            PromiseResult::Successful(_) => {}
            PromiseResult::Failed => {
                self.balance += amount.0;
            }
        };
    }
}
