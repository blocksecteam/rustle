# Example of the detector `nft-approval-check`

This example is based on the implementation of the [near-contract-standards](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/core/core_impl.rs). However, a new function [`internal_transfer_unsafe`](src/lib.rs#L256) is added. In this function, the check of `approval_id` is removed, which is incorrect.

The interface `nft_transfer` will call this new unsafe `internal_transfer_unsafe` while the `nft_transfer_call` will still use the safe `internal_transfer`. So the **Rustle** will mark the `nft_transfer` as an unsafe one.
