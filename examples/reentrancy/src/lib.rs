use near_contract_standards::fungible_token::core::ext_ft_core;
use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::{env, ext_contract, log, Promise, PromiseResult};
use near_sdk::{near_bindgen, AccountId, Gas};

pub const TGAS: u64 = 1_000_000_000_000;
const GAS_FOR_FT_TRANSFER_CALL: Gas = Gas(30 * TGAS);
const GAS_FOR_FT_RESOLVE_TRANSFER: Gas = Gas(10 * TGAS);

#[ext_contract(ext_self)]
pub trait SelfContract {
    fn callback_withdraw(&mut self, amount: U128) -> bool;
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct VictimContract {
    token_id: AccountId,
    depositor: AccountId,
    balance: u128,
}

impl Default for VictimContract {
    fn default() -> Self {
        Self {
            balance: 100,
            token_id: "ft_token.test.near".parse().unwrap(),
            depositor: "attacker.test.near".parse().unwrap(),
        }
    }
}

#[near_bindgen]
impl VictimContract {
    pub fn withdraw(&mut self, amount: U128) -> Promise {
        log!("victim::withdraw :{:?}", env::block_height());

        assert!(self.balance >= amount.into(), "insufficient balance");

        ext_ft_core::ext(self.token_id.clone())
            .with_attached_deposit(1)
            .with_static_gas(GAS_FOR_FT_TRANSFER_CALL)
            .ft_transfer_call(self.depositor.clone(), amount, None, "".to_string())
            .then(
                ext_self::ext(env::current_account_id())
                    .with_static_gas(GAS_FOR_FT_RESOLVE_TRANSFER)
                    .with_attached_deposit(0)
                    .callback_withdraw(amount),
            )
    }

    pub fn get_balance(&self) -> U128 {
        U128(self.balance)
    }

    /// Here `callback_withdraw` changes balance only on `ft_transfer_call` successes.
    ///
    /// However, if the attacker call the `withdraw` again, the balance has not been changed yet, therefore, attacker
    /// can withdraw again.
    ///
    /// It is recommended to change the `balance` before calling `ft_transfer_call`, and restore the `balance` only when
    /// it fails
    #[private]
    pub fn callback_withdraw(&mut self, amount: U128) {
        log!("victim::callback_withdraw :{:?}", env::block_height());

        match env::promise_result(0) {
            PromiseResult::NotReady => unreachable!(),
            PromiseResult::Successful(_) => {
                self.balance -= amount.0;
            }
            PromiseResult::Failed => {}
        };
    }
}
