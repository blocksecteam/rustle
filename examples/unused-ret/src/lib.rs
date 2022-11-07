use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedSet;
use near_sdk::{near_bindgen, AccountId, BorshStorageKey};

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Users,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    users: UnorderedSet<AccountId>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            users: UnorderedSet::new(StorageKey::Users),
        }
    }
}

#[near_bindgen]
impl Contract {
    /// If `self.users` did not have `user` present, `true` is returned. If `self.users` did have `user` present, `false` is returned
    fn add_single_user(&mut self, user: &AccountId) -> bool {
        self.users.insert(user)
    }

    /// Returns whether `user` was present in `self.users`
    fn remove_single_user(&mut self, user: &AccountId) -> bool {
        self.users.remove(user)
    }

    pub fn add_users(&mut self, users: Vec<AccountId>) {
        for user in users {
            self.add_single_user(&user);
        }
    }

    pub fn remove_users(&mut self, users: Vec<AccountId>) {
        for user in users {
            self.remove_single_user(&user);
        }
    }
}
