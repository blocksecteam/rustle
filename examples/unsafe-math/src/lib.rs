use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::near_bindgen;

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct CalculatorContract {}

impl Default for CalculatorContract {
    fn default() -> Self {
        Self {}
    }
}

#[near_bindgen]
impl CalculatorContract {
    pub fn sum(&self, a: i32, b: i32) -> i32 {
        a + b
    }

    pub fn sub(&self, a: i32, b: i32) -> i32 {
        a - b
    }

    pub fn mul(&self, a: i32, b: i32) -> i32 {
        a * b
    }
}
