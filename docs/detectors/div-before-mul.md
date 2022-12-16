## Division before multiplication

### Configuration

* detector id: `div-before-mul`
* severity: medium

### Description

Find division before multiplication like `3 / 2 * 6 == 6`, which has a different result compared with `3 * 6 / 2 == 9`. This can result in precision loss.

It is suggested to conduct multiplication before division.

### Sample code

Below shows an example.  The formula `farm.amount / (farm.end_date - farm.start_date) as u128` will be floored and can result in precision loss.

```rust
// https://github.com/linear-protocol/LiNEAR/blob/fdbacc68c98205cba9f42c130d464ab3114257b6/contracts/linear/src/farm.rs#L125
let reward_per_session =
    farm.amount / (farm.end_date - farm.start_date) as u128 * SESSION_INTERVAL as u128;
```

It's recommended to use division after multiplication to mitigate precision loss:

```rust
let reward_per_session =
    farm.amount * SESSION_INTERVAL as u128 / (farm.end_date - farm.start_date) as u128;
```
