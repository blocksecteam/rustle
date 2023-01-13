## NEP-141 transfer doesn't panic on unregistered receiver accounts

### Configuration

* detector id: `unregistered-receiver`
* severity: medium

### Description

According to the [implementation](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/fungible_token/core_impl.rs#L58) of [NEP-141](https://github.com/near/NEPs/blob/master/neps/nep-0141.md), a transfer receiver that has not been registered should result in panic.

Developers may try to register a new account for the receiver without asking for the storage fee, which may lead to DoS. A possible implementation of account registration is in [storage_impl.rs](https://github.com/near/near-sdk-rs/blob/1859ce4c201d2a85fbe921fdada1df59b00d2d8c/near-contract-standards/src/fungible_token/storage_impl.rs#L45)

### Sample code

The code segment below shows two ways of unwrapping the balance of an account (i.e., the receiver). The safe one will panic when the account is not registered, while the unsafe one will return a default balance of 0 without panic. In the unsafe case, a new key with `AccountId == account_id` will be inserted into the `accounts` map, whose storage fee is paid by the sponsor of the `Contract`.

For the full sample code, refer to [examples/unregistered-receiver](/examples/unregistered-receiver).

```rust
#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    accounts: UnorderedMap<AccountId, Balance>,
    total_supply: Balance,
}

impl Contract {
    pub fn internal_unwrap_balance_safe(&self, account_id: &AccountId) -> Balance {
        self.accounts
            .get(account_id)
            .unwrap_or_else(|| env::panic_str("Account is not registered"))
    }
    pub fn internal_unwrap_balance_unsafe(&self, account_id: &AccountId) -> Balance {
        self.accounts.get(account_id).unwrap_or(0)
    }
}
```
