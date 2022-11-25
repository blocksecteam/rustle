## Lack of check for storage expansion

### Configuration

* detector id: `storage-gas`
* severity: low

### Description

Each time the state grows, it should be ensured that there is enough Balance to cover the expansion.

### Sample code

In the following code, storage usage expands after the insertion to `self.banks`, the difference should be checked to ensure the storage fee attached by the caller is enough.

```rust
let prev_storage = env::storage_usage();

self.banks.insert(&Bank {
    owner: bank.owner,
    balance: bank.balance.0,
});

assert!(
    env::attached_deposit() > ((env::storage_usage() - prev_storage) as u128 * env::storage_byte_cost()),
    "insufficient storage gas"
);
```
