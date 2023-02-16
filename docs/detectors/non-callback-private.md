## Improper `#[private]` use in non-callback function

### Configuration

* detector id: `non-callback-private`
* severity: low

### Description

Macro `#[private]` is usually used in callback function to ensure `current_account_id` equals `predecessor_account_id`. It shouldn't be used in non-callback functions. Otherwise, a public function will be an internal one.

### Sample code

```rust
#[near_bindgen]
impl Pool {
    #[private]
    pub fn get_tokens(&self) -> &[AccountId] {
        &self.token_account_ids
    }
}
```

`get_tokens` should be a public interface (a public function without `#[private]` macro decorated) for everyone to check all token types in a pool. It shouldn't be decorated with `#[private`]`.
