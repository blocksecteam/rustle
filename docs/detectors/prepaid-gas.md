## Lack of check for prepaid gas in `ft_transfer_call`

### Configuration

* detector id: `prepaid-gas`
* severity: low

### Description

In `ft_transfer_call`, the contract should check whether the prepaid_gas is enough for `ft_on_transfer` and `ft_resolve_transfer` before launching a cross-contract invocation.

### Sample code

This sample comes from [near-contract-standards](https://github.com/near/near-sdk-rs/tree/master/near-contract-standards), prepaid gas should be checked before making a cross-contract invocation in `ft_transfer_call`.

```rust
// https://github.com/near/near-sdk-rs/blob/770cbce018a1b6c49d58276a075ace3da96d6dc1/near-contract-standards/src/fungible_token/core_impl.rs#L136
fn ft_transfer_call(
    &mut self,
    receiver_id: AccountId,
    amount: U128,
    memo: Option<String>,
    msg: String,
) -> PromiseOrValue<U128> {
    require!(env::prepaid_gas() > GAS_FOR_FT_TRANSFER_CALL, "More gas is required");
    // ...
}
```
