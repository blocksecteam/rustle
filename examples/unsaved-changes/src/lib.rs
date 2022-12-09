use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::json_types::I128;
use near_sdk::{env, near_bindgen, AccountId, BorshStorageKey};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Accounts,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    accounts: UnorderedMap<AccountId, i128>,
}

#[near_bindgen]
impl Contract {
    #[init]
    pub fn new() -> Self {
        Self {
            accounts: UnorderedMap::new(StorageKey::Accounts),
        }
    }
}

impl Contract {
    pub fn register(&mut self) {
        self.accounts.insert(&env::predecessor_account_id(), &0);
    }

    pub fn modify_safe(&mut self, change: I128) {
        let mut balance = self
            .accounts
            .get(&env::predecessor_account_id())
            .unwrap_or_else(|| env::panic_str("Account is not registered"));

        balance = balance
            .checked_add(change.0)
            .unwrap_or_else(|| env::panic_str("Overflow"));

        self.accounts
            .insert(&env::predecessor_account_id(), &balance);
    }

    #[allow(unused)]
    pub fn modify_unsafe(&mut self, change: I128) {
        let mut balance = self
            .accounts
            .get(&env::predecessor_account_id())
            .unwrap_or_else(|| env::panic_str("Account is not registered"));

        balance = balance
            .checked_add(change.0)
            .unwrap_or_else(|| env::panic_str("Overflow"));

        // self.accounts
        //     .insert(&env::predecessor_account_id(), &balance);
    }
}
