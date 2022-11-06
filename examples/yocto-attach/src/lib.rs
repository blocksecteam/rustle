use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::{assert_one_yocto, env, near_bindgen, require, AccountId};

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    owner_id: AccountId,
    fee_rate: u32,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            owner_id: AccountId::new_unchecked("owner.near".to_string()),
            fee_rate: 0,
        }
    }
}

#[near_bindgen]
impl Contract {
    /// move owner authority to another account
    #[payable]
    pub fn set_owner(&mut self, owner_id: AccountId) {
        assert_one_yocto();
        require!(
            env::predecessor_account_id() == self.owner_id,
            "only owner can call this function"
        );
        self.owner_id = owner_id.clone();
    }

    /// change fee rate
    #[payable]
    pub fn set_fee_rate(&mut self, fee_rate: u32) {
        // assert_one_yocto();
        require!(
            env::predecessor_account_id() == self.owner_id,
            "only owner can call this function"
        );
        self.fee_rate = fee_rate.clone();
    }
}
