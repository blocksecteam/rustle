use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedSet;
use near_sdk::json_types::U128;
use near_sdk::serde::{Deserialize, Serialize};
use near_sdk::{env, near_bindgen, AccountId, Balance, BorshStorageKey};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Banks,
}

#[derive(BorshDeserialize, BorshSerialize)]
pub struct Bank {
    owner: AccountId,
    balance: Balance,
}

#[derive(BorshDeserialize, BorshSerialize, Serialize, Deserialize)]
#[serde(crate = "near_sdk::serde")]
pub struct BankInterface {
    owner: AccountId,
    balance: U128,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    banks: UnorderedSet<Bank>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            banks: UnorderedSet::new(StorageKey::Banks),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn create_bank_safe(&mut self, bank: BankInterface) {
        let prev_storage = env::storage_usage();

        self.banks.insert(&Bank {
            owner: bank.owner,
            balance: bank.balance.0,
        });

        assert!(
            env::attached_deposit()
                > ((env::storage_usage() - prev_storage) as u128 * env::storage_byte_cost()),
            "insufficient storage gas"
        );
    }

    pub fn create_bank_unsafe(&mut self, bank: BankInterface) {
        self.banks.insert(&Bank {
            owner: bank.owner,
            balance: bank.balance.0,
        });
    }
}
