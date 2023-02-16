## Duplicate id uses in collections

### Configuration

* detector id: `dup-collection-id`
* severity: medium

### Description

Collections in NEAR SDK use function `new` to initialize itself.

```rust
pub fn new<S>(prefix: S) -> Self
where
    S: IntoStorageKey,
```

The argument `prefix` is an identifier for collection. Every collection should have an id different from the id of other collections.

### Sample code

```rust
#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    orders: UnorderedMap<u16, Order>,
    users: UnorderedSet<AccountId>,
}

#[near_bindgen]
impl Contract {
    #[init]
    pub fn new() -> Self {
        Self {
            orders: UnorderedMap::new(StorageKey::Orders),
            users: UnorderedSet::new(StorageKey::Orders), // Should use `StorageKey::Users` here
        }
    }
}
```

In this sample, `orders` and `users` share the same id, which may result in unexpected behavior.
