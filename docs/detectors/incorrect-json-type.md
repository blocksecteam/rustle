## Use of incorrect json type

### Configuration

* detector id: `incorrect-json-type`
* severity: high

### Description

Don't use type `i64`, `i128`, `u64`, or `u128` as the parameters or return values of public interfaces (public functions without `#[private]` macro in a `#[near_bindgen]` struct).

This is because the largest integer that json can support is 2\^53-1.

Check https://2ality.com/2012/04/number-encoding.html for more details.

Type `I64`, `I128`, `U64`, and `U128` in Near SDK are recommended for developers.

### Sample code

```rust
// https://github.com/ref-finance/ref-contracts/blob/c580d8742d80033a630a393180163ab70f9f3c94/ref-exchange/src/views.rs#L15
pub struct ContractMetadata {
    pub version: String,
    pub owner: AccountId,
    pub guardians: Vec<AccountId>,
    pub pool_count: u64,  // should use `U64`
    pub state: RunningState,
    pub exchange_fee: u32,
    pub referral_fee: u32,
}

// https://github.com/ref-finance/ref-contracts/blob/c580d8742d80033a630a393180163ab70f9f3c94/ref-exchange/src/views.rs#L171
#[near_bindgen]
impl Contract {
    pub fn metadata(&self) -> ContractMetadata {  // `u64` in return type
        // ...
    }
}
```

In this sample, the `metadata` uses struct `ContractMetadata` which contains `pool_count: u64` in its return value. Since return values of public functions are sent back to the front end using json and the value of `pool_count` may exceed 2\^53-1. Therefore, string type `U64` from Near SDK should be used.
