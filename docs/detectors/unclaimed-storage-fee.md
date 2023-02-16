## Missing check for the storage fee before trying to unregister the storage

### Configuration

* detector id: `unclaimed-storage-fee`
* severity: low

### Description

According to [NEP-145](https://github.com/near/NEPs/blob/master/neps/nep-0145.md#5-account-gracefully-closes-registration), if the owner of an account tries to close the account, he needs to unregister the storage with a zero balance unless the `force` flag is set. So the implementation of NEP-145 should comply with this rule.

### Sample code

Here is a wrong example of `storage_unregister`:

```rust
fn storage_unregister(&mut self, force: Option<bool>) -> bool {
    assert_one_yocto();
    let account_id = env::predecessor_account_id();
    if let Some(balance) = self.accounts.get(&account_id) {
        self.accounts.remove(&account_id);
        self.total_supply -= balance;
        Promise::new(account_id.clone()).transfer(self.storage_balance_bounds().min.0 + 1);
        return true;
    } else {
        log!("The account {} is not registered", &account_id);
    }
    false
}
```

This one will remove the account from `accounts` without checking the `balance`. So the correct version is:

```rust
fn storage_unregister(&mut self, force: Option<bool>) -> bool {
    assert_one_yocto();
    let account_id = env::predecessor_account_id();
    let force = force.unwrap_or(false);
    if let Some(balance) = self.accounts.get(&account_id) {
        if balance == 0 || force {
            self.accounts.remove(&account_id);
            self.total_supply -= balance;
            Promise::new(account_id.clone()).transfer(self.storage_balance_bounds().min.0 + 1);
            true
        } else {
            env::panic_str(
                "Can't unregister the account with the positive balance without force",
            )
        }
    } else {
        log!("The account {} is not registered", &account_id);
        false
    }
}
```
