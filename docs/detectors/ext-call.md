## Cross-contract invocations

### Configuration

* detector id: `ext-call`
* severity: info

### Description

Find all cross-contract invocations.

We provide this detector to help users quickly locate all cross-contract invocations for further analysis.

### Sample code

```rust
// https://docs.near.org/sdk/rust/cross-contract/callbacks#calculator-example
#[ext_contract(ext_calculator)]
trait Calculator {
    fn sum(&self, a: U128, b: U128) -> U128;
}

#[near_bindgen]
impl Contract {
    pub fn sum_a_b(&mut self, a: U128, b: U128) -> Promise {
        let calculator_account_id: AccountId = "calc.near".parse().unwrap();
        ext_calculator::ext(calculator_account_id).sum(a, b)  // run sum(a, b) on remote
    }
}
```

> Macro `#[ext_contract(...)]` will convert a Rust trait to a module with static methods, which allows users to make cross-contract invocation. Each of these static methods takes positional arguments defined by the Trait, then the `receiver_id`, the attached deposit, and the amount of gas and return a new `Promise`.

In this sample, the contract can get the sum of `a` and `b` with the cross-contract invocation (i.e., `ext_calculator::ext(calculator_account_id).sum(a, b)`).
