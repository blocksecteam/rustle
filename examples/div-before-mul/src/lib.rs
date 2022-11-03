use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::json_types::U128;
use near_sdk::{near_bindgen, AccountId, Balance, BorshStorageKey, Timestamp};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Stakes,
}

const REWARD_RATE: u64 = 1_000_000_000;

#[derive(BorshDeserialize, BorshSerialize)]
pub struct Stake {
    owner: AccountId,
    amount: Balance,
    pub start_date: Timestamp,
    pub end_date: Timestamp,
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
    pub fn calculate_reward(&self, id: u16) -> U128 {
        let stake = self.stakes.get(&id).unwrap();

        let amount_per_time_unit = stake.amount / (stake.end_date - stake.start_date) as u128;
        let reward = amount_per_time_unit * REWARD_RATE as u128;
        U128::from(reward)
    }
}
