# Example of the detector `nft-owner-check`

This example is based on the implementation of the [near-contract-standards](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/approval/approval_impl.rs). However, the owner check of function [`nft_revoke_all`](src/lib.rs#L171) is removed. The interface will become unsafe since anyone can call this function to revoke all approvals to the NFT.
