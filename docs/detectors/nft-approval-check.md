## Lack of check approval id during NFT transfer

### Configuration

* detector id: `nft-approval-check`
* severity: high

### Description

During an NFT transfer (i.e, `nft_transfer` and `nft_transfer_call`), if the sender of the transfer is not the owner of the NFT, the `approval_id` of the transfer should be checked to make sure the sender has the authority to launch this transfer. Without this check, anyone can transfer an NFT to other accounts without approval.

### Sample code

Code in [near-contract-standards](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/core/core_impl.rs#L212) shows the correct implementation.

```rust
// https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/core/core_impl.rs#L215
let app_acc_ids =
    approved_account_ids.as_ref().unwrap_or_else(|| env::panic_str("Unauthorized"));

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
```
