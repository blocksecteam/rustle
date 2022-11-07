## Unhandled promise's result

### Configuration

* detector id: `unhandled-promise`
* severity: high

### Description

Promise results should always be handled by a callback function or another promise. It is not recommended to leave promises unhandled in contracts. Otherwise, the changed state cannot be rolled back if the promise failed.

### Sample code

```rust
token.ft_transfer_call(receiver, U128(amount), None, "".to_string());
```

In this sample, the contract invokes `ft_transfer_call` but doesn't handle its promise result with a specified callback function (e.g., `resolve_transfer`). Therefore, The contract won't know whether the transfer is successful. And if the transfer is failed, contract states will not be rolled back.
