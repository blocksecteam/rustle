## Unsafe math without overflow check

### Configuration

* detector id: `unsafe-math`
* severity: high

### Description

Enable overflow checks for all arithmetic operations. Otherwise, overflow can occur, resulting in incorrect results.

Overflow checks in NEAR contracts can be implemented with two different methods.

1. \[Recommended\] Turn on the `overflow-checks` in the cargo manifest. In this case, it's okay to use `+`, `-` and `*` for arithmetic operations.
2. Use safe math functions (e.g., `checked_xxx()`) to do arithmetic operations.

### Sample code

In this example, since the `overflow-checks` flag is turned off in the cargo manifest, the use of `+` may lead to overflow.

```toml
[profile.xxx]  # `xxx` equals `dev` or `release`
overflow-checks = false
```

```rust
let a = b + c;
```

The following code is recommended to do the addition operation with `overflow-checks=false`

```rust
let a = b.checked_add(c);
```
