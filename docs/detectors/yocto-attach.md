## Lack of two-factor authentication

### Configuration

* detector id: `yocto-attach`
* severity: medium

### Description

Privileged functions should check whether one yocto NEAR is attached. This will enable the 2FA in the NEAR wallet for security concerns.

This can be implemented in the contract by adding `assert_one_yocto`, which is recommended for all privileged functions.

Note: It's hard to locate all the privileged functions without semantic information. Currently, **Rustle** considers all functions with `predecessor_account_id` involved in comparisons as privileged ones.

### Sample code

```rust
pub(crate) fn assert_owner(&self) {
    assert_eq!(
        env::predecessor_account_id(),
        self.owner_id,
        "{}", ERR100_NOT_ALLOWED
    );
}

// https://github.com/ref-finance/ref-contracts/blob/536a60c842e018a535b478c874c747bde82390dd/ref-exchange/src/owner.rs#L16
#[payable]
pub fn set_owner(&mut self, owner_id: ValidAccountId) {
    assert_one_yocto();
    self.assert_owner();
    self.owner_id = owner_id.as_ref().clone();
}
```

In this sample, `set_owner` is a privileged function and should only be invoked by the contract owner. Function `assert_owner` ensures the privilege by checking its `predecessor_account_id`. This function should have a 2FA for security concerns, and this can be implemented by adding an `assert_one_yocto` invocation in the function.
