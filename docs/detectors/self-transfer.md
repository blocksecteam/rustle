## Lack of check for self-transfer

### Configuration

* detector id: `self-transfer`
* severity: high

### Description

Before transferring tokens to the receiver, the contract should check whether the receiver is the sender itself. Otherwise, attackers may mint infinite tokens by abusing this vulnerability.

Check [Stader\_\_NEAR Incident Report — 08/16/2022 | by Stader Labs | Medium](https://blog.staderlabs.com/stader-near-incident-report-08-16-2022-afe077ffd549) and [Ensure that sender and receiver are not the same in ft_transfer · stader-labs/near-liquid-token@04480ab (github.com)](https://github.com/stader-labs/near-liquid-token/commit/04480abe4585b75a663e1d7fae673da7d7fe7ea3) for more details.

### Sample code

Code in [near-contract-standards](https://github.com/near/near-sdk-rs/tree/master/near-contract-standards) shows the correct implementation that the receiver should not be the sender.

```rust
// https://github.com/near/near-sdk-rs/blob/770cbce018a1b6c49d58276a075ace3da96d6dc1/near-contract-standards/src/fungible_token/core_impl.rs#L121
fn ft_transfer(&mut self, receiver_id: AccountId, amount: U128, memo: Option<String>) {
    // ...
    let sender_id = env::predecessor_account_id();
    let amount: Balance = amount.into();
    self.internal_transfer(&sender_id, &receiver_id, amount, memo);
}

// https://github.com/near/near-sdk-rs/blob/770cbce018a1b6c49d58276a075ace3da96d6dc1/near-contract-standards/src/fungible_token/core_impl.rs#L93
pub fn internal_transfer(&mut self, sender_id: &AccountId, receiver_id: &AccountId, amount: Balance, memo: Option<String>) {
    require!(sender_id != receiver_id, "Sender and receiver should be different");
    // ...
}
```
