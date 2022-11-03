use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::UnorderedMap;
use near_sdk::{env, ext_contract, require, BorshStorageKey, Promise, PromiseResult};
use near_sdk::{near_bindgen, Gas};

pub const TGAS: u64 = 1_000_000_000_000;
const GAS_FOR_EXT_CALL: Gas = Gas(10 * TGAS);
const GAS_FOR_CALLBACK: Gas = Gas(10 * TGAS);

#[derive(BorshStorageKey, BorshSerialize)]
pub(crate) enum StorageKey {
    Users,
}

#[ext_contract(ext_score)]
trait ScoreContract {
    fn use_score(uid: u16, amount: u32);
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    user_score: UnorderedMap<u16, u32>,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            user_score: UnorderedMap::new(StorageKey::Users),
        }
    }
}

#[near_bindgen]
impl Contract {
    pub fn query_score(&mut self, uid: u16) -> u32 {
        self.user_score.get(&uid).unwrap()
    }

    pub fn subtract_score(&mut self, uid: u16, amount: u32) -> Promise {
        require!(self.user_score.get(&uid).is_some());
        require!(self.user_score.get(&uid).unwrap() > amount);

        self.user_score
            .insert(&uid, &(self.user_score.get(&uid).unwrap() - amount));

        ext_score::ext("score.near".parse().unwrap())
            .with_attached_deposit(1)
            .with_static_gas(GAS_FOR_EXT_CALL)
            .use_score(uid.clone(), amount.clone())
            .then(
                Self::ext(env::current_account_id())
                    .with_static_gas(GAS_FOR_CALLBACK)
                    .use_score_callback(
                        uid.clone(),
                        amount.clone(), // original amount
                    ),
            )
    }

    /// Lack of `#[private]` in this callback function
    ///
    /// Attacker can exploit this function by call it after a **failed**
    /// cross-contract invocation with an arbitrary `amount`
    pub fn use_score_callback(&mut self, uid: u16, amount: u32) {
        match env::promise_result(0) {
            PromiseResult::NotReady => unreachable!(),
            PromiseResult::Successful(_) => {}
            PromiseResult::Failed => {
                self.user_score.insert(&uid, &amount);
            }
        }
    }
}

// #[cfg(not(target_arch = "wasm32"))]
// #[cfg(test)]
// mod test {
//     use near_sdk::test_utils::{accounts, VMContextBuilder};
//     use near_sdk::{testing_env, AccountId};

//     use super::*;

//     fn get_context(predecessor_account_id: AccountId) -> VMContextBuilder {
//         let mut builder = VMContextBuilder::new();
//         builder
//             .current_account_id(accounts(0))
//             .signer_account_id(predecessor_account_id.clone())
//             .predecessor_account_id(predecessor_account_id);
//         builder
//     }

//     #[test]
//     fn exploit() {
//         let mut context = get_context(accounts(1));
//         // Initialize the mocked blockchain
//         testing_env!(context.build());

//         // Set the testing environment for the subsequent calls
//         testing_env!(context.predecessor_account_id(accounts(1)).build());

//         let mut contract = Contract::default();
//         let user_id = 1;

//         let original_score: u32 = 100;
//         let expected_score: u32 = 300;
//         contract.user_score.insert(&user_id, &original_score);
//         println!(
//             "user_id's score before exploiting: {:?}",
//             contract.query_score(user_id.clone())
//         );

//         contract.use_score_callback(user_id, expected_score);

//         println!(
//             "user_id's score after exploiting: {:?}",
//             contract.query_score(user_id.clone())
//         );
//     }
// }
