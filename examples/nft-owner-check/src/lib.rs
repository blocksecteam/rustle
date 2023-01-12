use near_contract_standards::non_fungible_token::{
    approval::NonFungibleTokenApproval, bytes_for_approved_account_id, metadata::TokenMetadata,
    refund_approved_account_ids, refund_approved_account_ids_iter, refund_deposit, TokenId,
};
use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::{LookupMap, TreeMap, UnorderedSet};
use near_sdk::{
    assert_one_yocto, env, ext_contract, require, AccountId, BorshStorageKey, Gas, IntoStorageKey,
    Promise, StorageUsage,
};
use std::collections::HashMap;

const GAS_FOR_NFT_APPROVE: Gas = Gas(10_000_000_000_000);

fn expect_token_found<T>(option: Option<T>) -> T {
    option.unwrap_or_else(|| env::panic_str("Token not found"))
}

fn expect_approval<T>(option: Option<T>) -> T {
    option.unwrap_or_else(|| env::panic_str("next_approval_by_id must be set for approval ext"))
}

pub(crate) fn assert_at_least_one_yocto() {
    require!(
        env::attached_deposit() >= 1,
        "Requires attached deposit of at least 1 yoctoNEAR"
    )
}

#[ext_contract(ext_nft_approval_receiver)]
pub trait NonFungibleTokenApprovalReceiver {
    fn nft_on_approve(
        &mut self,
        token_id: TokenId,
        owner_id: AccountId,
        approval_id: u64,
        msg: String,
    ) -> near_sdk::PromiseOrValue<String>;
}

#[derive(BorshStorageKey, BorshSerialize)]
pub enum StorageKey {
    TokensPerOwner { account_hash: Vec<u8> },
}

#[derive(BorshDeserialize, BorshSerialize)]
pub struct NFT {
    pub owner_id: AccountId,

    pub extra_storage_in_bytes_per_token: StorageUsage,

    pub owner_by_id: TreeMap<TokenId, AccountId>,

    pub token_metadata_by_id: Option<LookupMap<TokenId, TokenMetadata>>,

    pub tokens_per_owner: Option<LookupMap<AccountId, UnorderedSet<TokenId>>>,

    pub approvals_by_id: Option<LookupMap<TokenId, HashMap<AccountId, u64>>>,
    pub next_approval_id_by_id: Option<LookupMap<TokenId, u64>>,
}

impl NFT {
    pub fn new<Q, R, S, T>(
        owner_by_id_prefix: Q,
        owner_id: AccountId,
        token_metadata_prefix: Option<R>,
        enumeration_prefix: Option<S>,
        approval_prefix: Option<T>,
    ) -> Self
    where
        Q: IntoStorageKey,
        R: IntoStorageKey,
        S: IntoStorageKey,
        T: IntoStorageKey,
    {
        let (approvals_by_id, next_approval_id_by_id) = if let Some(prefix) = approval_prefix {
            let prefix: Vec<u8> = prefix.into_storage_key();
            (
                Some(LookupMap::new(prefix.clone())),
                Some(LookupMap::new([prefix, "n".into()].concat())),
            )
        } else {
            (None, None)
        };

        let this = Self {
            owner_id,
            extra_storage_in_bytes_per_token: 0,
            owner_by_id: TreeMap::new(owner_by_id_prefix),
            token_metadata_by_id: token_metadata_prefix.map(LookupMap::new),
            tokens_per_owner: enumeration_prefix.map(LookupMap::new),
            approvals_by_id,
            next_approval_id_by_id,
        };
        this
    }
}

impl NonFungibleTokenApproval for NFT {
    fn nft_approve(
        &mut self,
        token_id: TokenId,
        account_id: AccountId,
        msg: Option<String>,
    ) -> Option<Promise> {
        assert_at_least_one_yocto();
        let approvals_by_id = self
            .approvals_by_id
            .as_mut()
            .unwrap_or_else(|| env::panic_str("NFT does not support Approval Management"));

        let owner_id = expect_token_found(self.owner_by_id.get(&token_id));

        require!(
            env::predecessor_account_id() == owner_id,
            "Predecessor must be token owner."
        );

        let next_approval_id_by_id = expect_approval(self.next_approval_id_by_id.as_mut());
        let approved_account_ids = &mut approvals_by_id.get(&token_id).unwrap_or_default();
        let approval_id: u64 = next_approval_id_by_id.get(&token_id).unwrap_or(1u64);
        let old_approval_id = approved_account_ids.insert(account_id.clone(), approval_id);

        approvals_by_id.insert(&token_id, approved_account_ids);

        next_approval_id_by_id.insert(&token_id, &(approval_id + 1));

        let storage_used = if old_approval_id.is_none() {
            bytes_for_approved_account_id(&account_id)
        } else {
            0
        };
        refund_deposit(storage_used);

        msg.map(|msg| {
            ext_nft_approval_receiver::ext(account_id)
                .with_static_gas(env::prepaid_gas() - GAS_FOR_NFT_APPROVE)
                .nft_on_approve(token_id, owner_id, approval_id, msg)
        })
    }

    fn nft_revoke(&mut self, token_id: TokenId, account_id: AccountId) {
        assert_one_yocto();
        let approvals_by_id = self.approvals_by_id.as_mut().unwrap_or_else(|| {
            env::panic_str("NFT does not support Approval Management");
        });

        let owner_id = expect_token_found(self.owner_by_id.get(&token_id));
        let predecessor_account_id = env::predecessor_account_id();

        require!(
            predecessor_account_id == owner_id,
            "Predecessor must be token owner."
        );

        if let Some(approved_account_ids) = &mut approvals_by_id.get(&token_id) {
            if approved_account_ids.remove(&account_id).is_some() {
                refund_approved_account_ids_iter(
                    predecessor_account_id,
                    core::iter::once(&account_id),
                );
                if approved_account_ids.is_empty() {
                    approvals_by_id.remove(&token_id);
                } else {
                    approvals_by_id.insert(&token_id, approved_account_ids);
                }
            }
        }
    }

    fn nft_revoke_all(&mut self, token_id: TokenId) {
        assert_one_yocto();
        let approvals_by_id = self.approvals_by_id.as_mut().unwrap_or_else(|| {
            env::panic_str("NFT does not support Approval Management");
        });

        // let owner_id = expect_token_found(self.owner_by_id.get(&token_id));
        let predecessor_account_id = env::predecessor_account_id();

        // require!(
        //     predecessor_account_id == owner_id,
        //     "Predecessor must be token owner."
        // );

        if let Some(approved_account_ids) = &mut approvals_by_id.get(&token_id) {
            refund_approved_account_ids(predecessor_account_id, approved_account_ids);
            approvals_by_id.remove(&token_id);
        }
    }

    fn nft_is_approved(
        &self,
        token_id: TokenId,
        approved_account_id: AccountId,
        approval_id: Option<u64>,
    ) -> bool {
        expect_token_found(self.owner_by_id.get(&token_id));

        let approvals_by_id = if let Some(a) = self.approvals_by_id.as_ref() {
            a
        } else {
            return false;
        };

        let approved_account_ids = if let Some(ids) = approvals_by_id.get(&token_id) {
            ids
        } else {
            return false;
        };

        let actual_approval_id = if let Some(id) = approved_account_ids.get(&approved_account_id) {
            id
        } else {
            return false;
        };

        if let Some(given_approval_id) = approval_id {
            &given_approval_id == actual_approval_id
        } else {
            true
        }
    }
}
