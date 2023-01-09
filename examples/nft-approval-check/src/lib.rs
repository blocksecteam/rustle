use near_contract_standards::non_fungible_token::{
    core::NonFungibleTokenCore, events::NftTransfer, metadata::TokenMetadata,
    refund_approved_account_ids, Token, TokenId,
};
use near_sdk::borsh::{self, BorshDeserialize, BorshSerialize};
use near_sdk::collections::{LookupMap, TreeMap, UnorderedSet};
use near_sdk::{
    assert_one_yocto, env, ext_contract, require, AccountId, BorshStorageKey, Gas, IntoStorageKey,
    PromiseOrValue, PromiseResult, StorageUsage,
};
use std::collections::HashMap;

const GAS_FOR_RESOLVE_TRANSFER: Gas = Gas(5_000_000_000_000);
const GAS_FOR_NFT_TRANSFER_CALL: Gas = Gas(25_000_000_000_000 + GAS_FOR_RESOLVE_TRANSFER.0);

#[ext_contract(ext_nft_receiver)]
pub trait NonFungibleTokenReceiver {
    fn nft_on_transfer(
        &mut self,
        sender_id: AccountId,
        previous_owner_id: AccountId,
        token_id: TokenId,
        msg: String,
    ) -> PromiseOrValue<bool>;
}
#[ext_contract(ext_nft_resolver)]
pub trait NonFungibleTokenResolver {
    fn nft_resolve_transfer(
        &mut self,
        previous_owner_id: AccountId,
        receiver_id: AccountId,
        token_id: TokenId,
        approvals: Option<HashMap<AccountId, u64>>,
    ) -> bool;
}

#[derive(BorshStorageKey, BorshSerialize)]
pub enum StorageKey {
    TokensPerOwner { account_hash: Vec<u8> },
}

#[derive(BorshDeserialize, BorshSerialize)]
pub struct NFT {
    // owner of contract
    pub owner_id: AccountId,

    // The storage size in bytes for each new token
    pub extra_storage_in_bytes_per_token: StorageUsage,

    // always required
    pub owner_by_id: TreeMap<TokenId, AccountId>,

    // required by metadata extension
    pub token_metadata_by_id: Option<LookupMap<TokenId, TokenMetadata>>,

    // required by enumeration extension
    pub tokens_per_owner: Option<LookupMap<AccountId, UnorderedSet<TokenId>>>,

    // required by approval extension
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

impl NonFungibleTokenCore for NFT {
    fn nft_transfer(
        &mut self,
        receiver_id: AccountId,
        token_id: TokenId,
        approval_id: Option<u64>,
        memo: Option<String>,
    ) {
        assert_one_yocto();
        let sender_id = env::predecessor_account_id();
        self.internal_transfer_unsafe(&sender_id, &receiver_id, &token_id, approval_id, memo);
    }

    fn nft_transfer_call(
        &mut self,
        receiver_id: AccountId,
        token_id: TokenId,
        approval_id: Option<u64>,
        memo: Option<String>,
        msg: String,
    ) -> PromiseOrValue<bool> {
        assert_one_yocto();
        require!(
            env::prepaid_gas() > GAS_FOR_NFT_TRANSFER_CALL,
            "More gas is required"
        );
        let sender_id = env::predecessor_account_id();
        let (old_owner, old_approvals) =
            self.internal_transfer(&sender_id, &receiver_id, &token_id, approval_id, memo);
        // Initiating receiver's call and the callback
        ext_nft_receiver::ext(receiver_id.clone())
            .with_static_gas(env::prepaid_gas() - GAS_FOR_NFT_TRANSFER_CALL)
            .nft_on_transfer(sender_id, old_owner.clone(), token_id.clone(), msg)
            .then(
                self::ext_nft_resolver::ext(env::current_account_id())
                    .with_static_gas(GAS_FOR_RESOLVE_TRANSFER)
                    .nft_resolve_transfer(old_owner, receiver_id, token_id, old_approvals),
            )
            .into()
    }

    fn nft_token(&self, token_id: TokenId) -> Option<Token> {
        let owner_id = self.owner_by_id.get(&token_id)?;
        let metadata = self
            .token_metadata_by_id
            .as_ref()
            .and_then(|by_id| by_id.get(&token_id));
        let approved_account_ids = self
            .approvals_by_id
            .as_ref()
            .and_then(|by_id| by_id.get(&token_id).or_else(|| Some(HashMap::new())));
        Some(Token {
            token_id,
            owner_id,
            metadata,
            approved_account_ids,
        })
    }
}

impl NFT {
    pub(crate) fn internal_transfer_unguarded(
        &mut self,
        #[allow(clippy::ptr_arg)] token_id: &TokenId,
        from: &AccountId,
        to: &AccountId,
    ) {
        // update owner
        self.owner_by_id.insert(token_id, to);

        // if using Enumeration standard, update old & new owner's token lists
        if let Some(tokens_per_owner) = &mut self.tokens_per_owner {
            // owner_tokens should always exist, so call `unwrap` without guard
            let mut owner_tokens = tokens_per_owner.get(from).unwrap_or_else(|| {
                env::panic_str("Unable to access tokens per owner in unguarded call.")
            });
            owner_tokens.remove(token_id);
            if owner_tokens.is_empty() {
                tokens_per_owner.remove(from);
            } else {
                tokens_per_owner.insert(from, &owner_tokens);
            }

            let mut receiver_tokens = tokens_per_owner.get(to).unwrap_or_else(|| {
                UnorderedSet::new(StorageKey::TokensPerOwner {
                    account_hash: env::sha256(to.as_bytes()),
                })
            });
            receiver_tokens.insert(token_id);
            tokens_per_owner.insert(to, &receiver_tokens);
        }
    }

    pub(crate) fn internal_transfer(
        &mut self,
        sender_id: &AccountId,
        receiver_id: &AccountId,
        #[allow(clippy::ptr_arg)] token_id: &TokenId,
        approval_id: Option<u64>,
        memo: Option<String>,
    ) -> (AccountId, Option<HashMap<AccountId, u64>>) {
        let owner_id = self
            .owner_by_id
            .get(token_id)
            .unwrap_or_else(|| env::panic_str("Token not found"));

        // clear approvals, if using Approval Management extension
        // this will be rolled back by a panic if sending fails
        let approved_account_ids = self
            .approvals_by_id
            .as_mut()
            .and_then(|by_id| by_id.remove(token_id));

        // check if authorized
        let sender_id = if sender_id != &owner_id {
            // if approval extension is NOT being used, or if token has no approved accounts
            let app_acc_ids = approved_account_ids
                .as_ref()
                .unwrap_or_else(|| env::panic_str("Unauthorized"));

            // Approval extension is being used; get approval_id for sender.
            let actual_approval_id = app_acc_ids.get(sender_id);

            // Panic if sender not approved at all
            if actual_approval_id.is_none() {
                env::panic_str("Sender not approved");
            }

            // If approval_id included, check that it matches
            require!(
                approval_id.is_none() || actual_approval_id == approval_id.as_ref(),
                format!(
                    "The actual approval_id {:?} is different from the given approval_id {:?}",
                    actual_approval_id, approval_id
                )
            );
            Some(sender_id)
        } else {
            None
        };

        require!(
            &owner_id != receiver_id,
            "Current and next owner must differ"
        );

        self.internal_transfer_unguarded(token_id, &owner_id, receiver_id);

        NFT::emit_transfer(&owner_id, receiver_id, token_id, sender_id, memo);

        // return previous owner & approvals
        (owner_id, approved_account_ids)
    }

    #[allow(unused)]
    pub(crate) fn internal_transfer_unsafe(
        &mut self,
        sender_id: &AccountId,
        receiver_id: &AccountId,
        #[allow(clippy::ptr_arg)] token_id: &TokenId,
        approval_id: Option<u64>,
        memo: Option<String>,
    ) -> (AccountId, Option<HashMap<AccountId, u64>>) {
        let owner_id = self
            .owner_by_id
            .get(token_id)
            .unwrap_or_else(|| env::panic_str("Token not found"));

        // clear approvals, if using Approval Management extension
        // this will be rolled back by a panic if sending fails
        let approved_account_ids = self
            .approvals_by_id
            .as_mut()
            .and_then(|by_id| by_id.remove(token_id));

        // check if authorized
        let sender_id = if sender_id != &owner_id {
            Some(sender_id)
        } else {
            None
        };

        require!(
            &owner_id != receiver_id,
            "Current and next owner must differ"
        );

        self.internal_transfer_unguarded(token_id, &owner_id, receiver_id);

        NFT::emit_transfer(&owner_id, receiver_id, token_id, sender_id, memo);

        // return previous owner & approvals
        (owner_id, approved_account_ids)
    }

    fn emit_transfer(
        owner_id: &AccountId,
        receiver_id: &AccountId,
        token_id: &str,
        sender_id: Option<&AccountId>,
        memo: Option<String>,
    ) {
        NftTransfer {
            old_owner_id: owner_id,
            new_owner_id: receiver_id,
            token_ids: &[token_id],
            authorized_id: sender_id.filter(|sender_id| *sender_id == owner_id),
            memo: memo.as_deref(),
        }
        .emit();
    }
}

impl NonFungibleTokenResolver for NFT {
    /// Returns true if token was successfully transferred to `receiver_id`.
    fn nft_resolve_transfer(
        &mut self,
        previous_owner_id: AccountId,
        receiver_id: AccountId,
        token_id: TokenId,
        approved_account_ids: Option<HashMap<AccountId, u64>>,
    ) -> bool {
        // Get whether token should be returned
        let must_revert = match env::promise_result(0) {
            PromiseResult::NotReady => unreachable!(),
            PromiseResult::Successful(value) => {
                if let Ok(yes_or_no) = near_sdk::serde_json::from_slice::<bool>(&value) {
                    yes_or_no
                } else {
                    true
                }
            }
            PromiseResult::Failed => true,
        };

        // if call succeeded, return early
        if !must_revert {
            return true;
        }

        // OTHERWISE, try to set owner back to previous_owner_id and restore approved_account_ids

        // Check that receiver didn't already transfer it away or burn it.
        if let Some(current_owner) = self.owner_by_id.get(&token_id) {
            if current_owner != receiver_id {
                // The token is not owned by the receiver anymore. Can't return it.
                return true;
            }
        } else {
            // The token was burned and doesn't exist anymore.
            // Refund storage cost for storing approvals to original owner and return early.
            if let Some(approved_account_ids) = approved_account_ids {
                refund_approved_account_ids(previous_owner_id, &approved_account_ids);
            }
            return true;
        };

        self.internal_transfer_unguarded(&token_id, &receiver_id, &previous_owner_id);

        // If using Approval Management extension,
        // 1. revert any approvals receiver already set, refunding storage costs
        // 2. reset approvals to what previous owner had set before call to nft_transfer_call
        if let Some(by_id) = &mut self.approvals_by_id {
            if let Some(receiver_approvals) = by_id.get(&token_id) {
                refund_approved_account_ids(receiver_id.clone(), &receiver_approvals);
            }
            if let Some(previous_owner_approvals) = approved_account_ids {
                by_id.insert(&token_id, &previous_owner_approvals);
            }
        }
        NFT::emit_transfer(&receiver_id, &previous_owner_id, &token_id, None, None);
        false
    }
}
