use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::{near_bindgen, AccountId, Balance, BorshStorageKey, Timestamp};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Stakes,
}

#[derive(BorshDeserialize, BorshSerialize)]
pub struct Stake {
    owner: AccountId,

    // both u64 and u128 are too large for json
    amount: Balance,           // u128
    pub start_date: Timestamp, // u64
    pub end_date: Timestamp,   // u64
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    stakes: UnorderedMap<u16, Stake>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            stakes: UnorderedMap::new(StorageKey::Stakes),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn get_stake_info(&self, id: u16) -> Stake {
        let stake = self.stakes.get(&id).unwrap();
        stake
    }
}
