## Lack of `#[private]` macro in callback functions

### Configuration

* detector id: `non-private-callback`
* severity: high

### Description

The callback function should be decorated with `#[private]` to ensure that it can only be invoked by the contract itself.

Without this macro, anyone can invoke the callback function, which is against the original design.

### Sample code

```rust
pub fn callback_stake(&mut self) { // this is a callback function
    // ...
}
```

Function `callback_stake` should be decorated with `#[private]`:

```rust
#[private]
pub fn callback_stake(&mut self) { // this is a callback function
    // ...
}
```

With this macro, only the contract itself can invoke `callback_stake`.
