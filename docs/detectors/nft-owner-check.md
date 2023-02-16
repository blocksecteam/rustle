## Lack of check approval id during NFT transfer

### Configuration

* detector id: `nft-owner-check`
* severity: high

### Description

In the design of the [NEP-178](https://github.com/near/NEPs/blob/master/neps/nep-0178.md), the owner of the NFT can approve or revoke approvals by using the specified interfaces (ie, `nft_approve`, `nft_revoke` and `nft_revoke_all`). An owner check should be implemented for these interfaces to make sure they are callable to the owner only, otherwise, anyone can modify the approvals of the NFT.

### Sample code

Code in [near-contract-standards](https://github.com/near/near-sdk-rs/blob/a903f8c44a7be363d960838d92afdb22d1ce8b87/near-contract-standards/src/non_fungible_token/approval/approval_impl.rs) shows the correct implementation.

```rust
// should be implemented for `nft_approve`, `nft_revoke` and `nft_revoke_all`
let owner_id = expect_token_found(self.owner_by_id.get(&token_id));
let predecessor_account_id = env::predecessor_account_id();

require!(predecessor_account_id == owner_id, "Predecessor must be token owner.");
```
