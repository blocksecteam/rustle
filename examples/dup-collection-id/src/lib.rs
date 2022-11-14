use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::{UnorderedMap, UnorderedSet};
use near_sdk::serde::Serialize;
use near_sdk::BorshStorageKey;
use near_sdk::{near_bindgen, AccountId};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Orders,
    #[allow(unused)]
    Users,
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
    users: UnorderedSet<AccountId>,
}

#[near_bindgen]
impl Contract {
    #[init]
    pub fn new() -> Self {
        Self {
            orders: UnorderedMap::new(StorageKey::Orders),
            users: UnorderedSet::new(StorageKey::Orders), // Should use `StorageKey::Users` here
        }
    }
}
