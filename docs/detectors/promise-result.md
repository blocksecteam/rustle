## Promise result

### Configuration

* detector id: `promise-result`
* severity: info

### Description

Find all uses of `env::promise_result`, which provides the result of promise execution.

This detector helps to quickly locate the logic of handling promise results.

### Sample code

```rust
// https://github.com/ref-finance/ref-contracts/blob/b33c3a488b18f9cff82a3fdd53bf65d6aac09e15/ref-exchange/src/lib.rs#L434
let cross_call_result = match env::promise_result(0) {
    PromiseResult::Successful(result) => result,
    _ => env::panic(ERR124_CROSS_CALL_FAILED.as_bytes()),
};
```

In this sample, the promise result is handled depending on its state (successful or not).
