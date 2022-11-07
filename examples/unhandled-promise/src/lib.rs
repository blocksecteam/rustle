use near_contract_standards::fungible_token::core::ext_ft_core;
use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::{near_bindgen, AccountId, Gas, Promise};

pub const TGAS: u64 = 1_000_000_000_000;
const GAS_FOR_FT_TRANSFER: Gas = Gas(10 * TGAS);

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    token_id: AccountId,
    depositor: AccountId,
    balance: u128,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            balance: 100,
            token_id: "ft_token.near".parse().unwrap(),
            depositor: "depositor.near".parse().unwrap(),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn withdraw_without_promise(&mut self, amount: U128) {
        assert!(self.balance >= amount.into(), "insufficient balance");
        self.balance -= amount.0;

        // no warning: not receiving promise so no needs to handle
        self.transfer_without_promise(amount);
    }

    pub fn withdraw_with_promise(&mut self, amount: U128) {
        assert!(self.balance >= amount.into(), "insufficient balance");
        self.balance -= amount.0;

        // warning: receive promise from `transfer_with_promise` but not handle it
        self.transfer_with_promise(amount);
    }

    fn transfer_without_promise(&mut self, amount: U128) {
        // warning: receive promise from `ft_transfer` but doesn't handle it
        ext_ft_core::ext(self.token_id.clone())
            .with_attached_deposit(1)
            .with_static_gas(GAS_FOR_FT_TRANSFER)
            .ft_transfer(self.depositor.clone(), amount, None);
    }

    fn transfer_with_promise(&mut self, amount: U128) -> Promise {
        // no warning: not handle, but return to `withdraw_with_promise`
        ext_ft_core::ext(self.token_id.clone())
            .with_attached_deposit(1)
            .with_static_gas(GAS_FOR_FT_TRANSFER)
            .ft_transfer(self.depositor.clone(), amount, None)
    }
}
