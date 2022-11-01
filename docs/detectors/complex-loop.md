## Complex logic inside a loop

### Configuration

* detector id: `complex-loop`
* severity: info

### Description

Find loops containing complex logic, which may result in DoS problems.

It's recommended to limit the iteration times to avoid the DoS problem.

### Sample code

```rust
impl User {
    pub fn complex_calc(&mut self) {
        // ...
    }
}

#[near_bindgen]
impl Contract {
    pub fn register_users(&mut self, user: User) {
        self.users.push(user);
    }

    pub fn update_all_users(&mut self) {
        for user in self.users.iter_mut() {
            user.complex_calc();
        }
    }
}
```

In this sample, users can be registered without any storage fee, so the `users` list can have a very long `len()`.

Assume the function `complex_calc` has many complex calculations and can cost a lot of gas.

In `update_all_users`, a loop is iterating over `users` with the function `complex_calc` called. This may drain out all the gas.

A malicious user can conduct the attack by registering tons of users without paying any storage fee.
