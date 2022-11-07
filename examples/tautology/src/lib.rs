use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::{env, near_bindgen, AccountId, BorshStorageKey};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Users,
}

#[derive(BorshDeserialize, BorshSerialize, PartialEq)]
pub enum Identity {
    Normal,
    Admin,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    users: UnorderedMap<AccountId, Identity>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            users: UnorderedMap::new(StorageKey::Users),
        }
    }
}

#[near_bindgen]
impl Contract {
    fn check_identity(&self, identity: Identity) -> bool {
        self.users
            .get(&env::predecessor_account_id())
            .unwrap_or_else(|| env::panic_str("AccountId not recorded"))
            == identity
    }

    pub fn normal_request(&self) {
        if true || self.check_identity(Identity::Normal) {
            unimplemented!();
        } else {
            env::panic_str("Invalid identity")
        }
    }

    pub fn admin_request(&self) {
        if self.check_identity(Identity::Admin) && false {
            unimplemented!();
        } else {
            env::panic_str("Invalid identity")
        }
    }
}
