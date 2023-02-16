## Find all public interfaces

### Configuration

* detector id: `public-interface`
* severity: info

### Description

Public interfaces are functions that can be called by others. Specifically, they are public functions without `#[private]` macro of a `#[near_bindgen]` struct.

### Sample code

```rust
#[near_bindgen]
impl Contract {
    pub fn withdraw(&mut self, amount: U128) -> Promise {
        unimplemented!()
    }

    pub fn check_balance(&self) -> U128 {
        unimplemented!()
    }

    #[private]
    pub fn callback_withdraw(&mut self, amount: U128) {
        unimplemented!()
    }

    fn sub_balance(&mut self, amount: u128) {
        unimplemented!()
    }
}
```

In this sample code, all four functions are methods of a `#[near_bindgen]` struct `Contract`. But only `withdraw` and `check_balance` are public interfaces. `callback_withdraw` is a public function decorated with `#[private]`, so it is an internal function. `sub_balance` is a private function of `Contract`.
