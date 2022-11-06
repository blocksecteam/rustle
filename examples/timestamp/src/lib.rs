use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::{env, near_bindgen, AccountId};

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct LotteryContract {
    users: Vec<AccountId>,
}

impl Default for LotteryContract {
    fn default() -> Self {
        Self { users: vec![] }
    }
}

#[near_bindgen]
impl LotteryContract {
    pub fn get_winner(&self) -> AccountId {
        let current_time = env::block_timestamp();
        self.generate_winner(current_time)
    }

    fn generate_winner(&self, time: u64) -> AccountId {
        self.users[((time as u128) % (self.users.len() as u128)) as usize].clone()
    }
}
