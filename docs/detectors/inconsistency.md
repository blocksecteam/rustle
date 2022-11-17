## Inconsistent symbols

### Configuration

* detector id: `inconsistency`
* severity: info

### Description

Symbols with similar but slightly different names may be misused. Such error is hard to find out manually but may result in critical results.

### Sample code

```rust
ext::ft_transfer(
    receiver_id,
    U128(amount),
    None,
    token_id,
    1,
    GAS_FOR_NFT_TRANSFER  // should use GAS_FOR_FT_TRANSFER here
)
```

In this sample, the contract should use `GAS_FOR_FT_TRANSFER` instead of `GAS_FOR_NFT_TRANSFER`. These two constants may vary a lot in value, and gas for the `ft_transfer` may be insufficient.
