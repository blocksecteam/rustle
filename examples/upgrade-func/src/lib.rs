use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::json_types::U128;
use near_sdk::serde::{Deserialize, Serialize};
use near_sdk::{env, near_bindgen, AccountId};

#[derive(Serialize, Deserialize)]
#[serde(crate = "near_sdk::serde")]
pub struct Meta {
    owner_id: AccountId,
    token_id: AccountId,
    depositor: AccountId,
    balance: U128,
}

#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    owner_id: AccountId,
    token_id: AccountId,
    depositor: AccountId,
    balance: u128,
}

impl Default for Contract {
    fn default() -> Self {
        Self {
            owner_id: "owner.near".parse().unwrap(),
            token_id: "ft_token.near".parse().unwrap(),
            depositor: "depositor.near".parse().unwrap(),
            balance: 100,
        }
    }
}

#[near_bindgen]
impl Contract {
    #[init(ignore_state)]
    #[private]
    pub fn migrate() -> Self {
        let contract: Contract = env::state_read().expect("Contract is not initialized");
        contract
    }

    pub fn get_meta(&self) -> Meta {
        Meta {
            owner_id: self.owner_id.clone(),
            balance: self.balance.into(),
            token_id: self.token_id.clone(),
            depositor: self.depositor.clone(),
        }
    }
}

#[cfg(target_arch = "wasm32")]
mod upgrade {
    use super::*;
    use near_sdk::{require, Gas};
    use near_sys as sys;

    pub const TGAS: u64 = 1_000_000_000_000;
    pub const GAS_FOR_COMPLETING_UPGRADE_CALL: Gas = Gas(10 * TGAS);
    pub const MIN_GAS_FOR_MIGRATE_CALL: Gas = Gas(10 * TGAS);
    pub const GAS_FOR_GET_META_CALL: Gas = Gas(5 * TGAS);

    #[no_mangle]
    pub fn upgrade() {
        env::setup_panic_hook();
        let contract: Contract = env::state_read().expect("Contract is not initialized");
        require!(
            env::predecessor_account_id() == contract.owner_id,
            "Only contract's owner can call upgrade"
        );
        let current_id = env::current_account_id().as_bytes().to_vec();
        let migrate_method_name = b"migrate".to_vec();
        let get_meta_method_name = b"get_meta".to_vec();
        unsafe {
            sys::input(0);
            let promise_id =
                sys::promise_batch_create(current_id.len() as _, current_id.as_ptr() as _);

            sys::promise_batch_action_deploy_contract(promise_id, u64::MAX as _, 0);

            let required_gas =
                env::used_gas() + GAS_FOR_COMPLETING_UPGRADE_CALL + GAS_FOR_GET_META_CALL;
            require!(
                env::prepaid_gas() >= required_gas + MIN_GAS_FOR_MIGRATE_CALL,
                "No enough gas to complete contract state migration"
            );
            let migrate_attached_gas = env::prepaid_gas() - required_gas;
            sys::promise_batch_action_function_call(
                promise_id,
                migrate_method_name.len() as _,
                migrate_method_name.as_ptr() as _,
                0 as _,
                0 as _,
                0 as _,
                migrate_attached_gas.0,
            );

            sys::promise_batch_action_function_call(
                promise_id,
                get_meta_method_name.len() as _,
                get_meta_method_name.as_ptr() as _,
                0 as _,
                0 as _,
                0 as _,
                GAS_FOR_GET_META_CALL.0,
            );
            sys::promise_return(promise_id);
        }
    }
}
