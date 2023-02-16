## Changes to collections are not saved

### Configuration

* detector id: `unsaved-changes`
* severity: high

### Description

[NEAR SDK](https://crates.io/crates/near-sdk) provides some map collections which can store key-value mappings. You can use `get` to get a value by its key and insert or overwrite the value by calling `insert` with a specified key. The collections use borsh to serialize and deserialize, when you want to make some changes to a collection, you need to `get` an existing value and make changes to it, then `insert` it into the collection again. Without the `insert` function, the changes will not be saved to the contract.

Note, since the [v4.1.0](https://docs.rs/near-sdk/4.1.0/near_sdk/index.html) of NEAR SDK, developers can use maps in [`near_sdk::store`](https://docs.rs/near-sdk/4.1.0/near_sdk/store/index.html) which will write back changes automatically to avoid this issue introduced by `near_sdk::collections`.

### Sample code

```rust
#[allow(unused)]
pub fn modify(&mut self, change: I128) {
    let mut balance = self
        .accounts
        .get(&env::predecessor_account_id())
        .unwrap_or_else(|| env::panic_str("Account is not registered"));

    balance = balance
        .checked_add(change.0)
        .unwrap_or_else(|| env::panic_str("Overflow"));

    // self.accounts
    //     .insert(&env::predecessor_account_id(), &balance);
}
```

In this sample, `balance` is a value from `self.accounts` indexed by the caller's account id, the contract makes some changes to it by calling `checked_add`. However, the changes are not saved without calling the `insert` function.
