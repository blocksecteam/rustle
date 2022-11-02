use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::near_bindgen;
use near_sdk::serde::{Deserialize, Serialize};

#[derive(BorshDeserialize, BorshSerialize, Serialize, Deserialize)]
#[serde(crate = "near_sdk::serde")]
pub struct User {
    id: u16,

    /// if you want to use amount lager than u32, please use [U64](near_sdk::json_types::U64) or [U128](near_sdk::json_types::U128)
    ///
    /// refer to [incorrect-json-type](https://github.com/blocksecteam/rustle/blob/main/docs/detectors/incorrect-json-type.md) for more details
    amount: u32,
}

impl User {
    /// a function containing complex logics
    ///
    /// here we only repeated additions and subtractions as an example of "complex"
    pub fn complex_calc(&mut self) {
        self.amount += 1;
        self.amount += 1;
        self.amount += 1;
        self.amount += 1;
        self.amount += 1;
        self.amount += 1;
        self.amount += 1;

        self.amount -= 1;
        self.amount -= 1;
        self.amount -= 1;
        self.amount -= 1;
        self.amount -= 1;
        self.amount -= 1;
        self.amount -= 1;
    }
}
#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    users: Vec<User>,
}

impl Default for Contract {
    fn default() -> Self {
        Self { users: vec![] }
    }
}

#[near_bindgen]
impl Contract {
    pub fn register_users(&mut self, user: User) {
        self.users.push(user);
    }

    pub fn update_all_users(&mut self) {
        for user in self.users.iter_mut() {
            user.complex_calc();
        }
    }
}
