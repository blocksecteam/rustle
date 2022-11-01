## Use of tautology in branch condition

### Configuration

* detector id: `tautology`
* severity: low

### Description

Find simple tautology (`true` or `false` involved in the condition) which makes a branch deterministic.

### Sample code

```rust
let ok: bool = check_state();
if true || ok {
    // ...
} else {
    // ...
}
```

In this demo, the code inside the `else` branch will never be executed. This is because `true || ok` is a tautology.
