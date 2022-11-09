use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::near_bindgen;

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    bonus_rate: u8,
    fee_rate: u8,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            bonus_rate: 2,
            fee_rate: 1,
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn calc_bonus(&self, amount: U128) -> U128 {
        // don't use `round`, you should use `floor` here
        U128((amount.0 as f64 * self.bonus_rate as f64 / 100.).round() as u128)
    }

    pub fn calc_fee(&self, amount: U128) -> U128 {
        // don't use `round`, you should use `ceil` here
        U128((amount.0 as f64 * self.fee_rate as f64 / 100.).round() as u128)
    }
}
