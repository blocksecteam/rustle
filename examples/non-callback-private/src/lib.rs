use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::serde::Serialize;
use near_sdk::{near_bindgen, AccountId};

#[derive(BorshDeserialize, BorshSerialize, Serialize, Clone)]
#[serde(crate = "near_sdk::serde")]
pub struct Token {
    token_id: AccountId,
    alias: String,
    supply: U128,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct PoolContract {
    token_list: Vec<Token>,
}

impl Default for PoolContract {
    fn default() -> Self {
        Self { token_list: vec![] }
    }
}

#[near_bindgen]
impl PoolContract {
    /// This function should be callable to every one to check the
    /// supported tokens in the pool
    ///
    /// It shouldn't be decorated with `#[private]`
    #[private]
    pub fn get_token_list(&self) -> Vec<Token> {
        self.token_list.clone()
    }
}
