## Panic in callback may lock the contract

### Configuration

* detector id: `lock-callback`
* severity: medium

### Description

It's not recommended to use `assert` or `require` or anything that will result in panic in a callback function.

In Near, the callback function needs to recover some state changes made by a failed promise. If the callback function panics, the state may not be fully recovered, which results in unexpected results.

### Sample code

```rust
fn process_order(&mut self, order_id: u32) -> Promise {
    let receiver = self.get_receiver(order_id);
    let amount = self.get_amount(order_id);
    self.delete_order(order_id);
    ext_contract::do_transfer(receiver, amount).then(ext_self::callback_transfer(order_id))
}

// ext_self::callback_transfer
#[private]
pub fn callback_transfer(&mut self, order_id: u32) {
    assert!(order_id > 0);

    match env::promise_result(0) {
        PromiseResult::NotReady => unreachable!(),
        PromiseResult::Successful(_) => {}
        PromiseResult::Failed => {
            self.recover_order(order_id);
        }
    };
}
```

In this sample, the contract first deletes the order and then transfers the token. If the transfer fails, the order should be recovered in `callback_transfer`.

However, there is an `assert` in the callback function, if it fails, the order wouldn't be recovered.
