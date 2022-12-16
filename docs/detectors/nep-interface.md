## Check the integrity of NEP interfaces

### Configuration

* detector id: `nep${id}-interface` (`${id}` is the NEP ID)
* severity: medium

### Description

[NEPs](https://github.com/near/NEPs) stand for Near Enhancement Proposals, which are some changes to the NEAR protocol specification and standards. Currently, there are NEPs for FT, NFT, MT and storage, listed in a [table](https://github.com/near/NEPs#neps).

To use this detector, you need to specify the NEP id in the detector id, for example:

```bash
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep141-interface  # Fungible Token Standard
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep145-interface  # Storage Management
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep171-interface  # Non Fungible Token Standard
```

### Sample code

The [example](/examples/nep-interface/) of detector `nep-interface` is adapted from the near-contract-standards.
