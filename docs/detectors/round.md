## Improper rounding without ceil or floor specified

### Configuration

* detector id: `round`
* severity: medium

### Description

Find if there is rounding used in arithmetic operations. Rounding without specifying direction may be exploited in DeFi. Refer to [How to Become a Millionaire, 0.000001 BTC at a Time (neodyme.io)](https://blog.neodyme.io/posts/lending_disclosure/) for more details.

Note: **Rustle** will not report rounding functions implemented by developers for specific purposes.

### Sample code

```rust
let fee = (amount * fee_rate).round();
```

In this sample, contract developers should not use `round` to calculate the fee. Instead, they should use `ceil` or `floor` to specify the rounding direction.
