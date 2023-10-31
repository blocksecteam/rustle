## 集合中的重复id使用

### 配置

* 检测器id：`dup-collection-id`
* 严重程度：中等

### 描述

NEAR SDK中的集合使用函数`new`来初始化自身。

```rust
pub fn new<S>(prefix: S) -> Self
where
    S: IntoStorageKey,
```

参数`prefix`是集合的标识符。每个集合的id应与其他集合的id不同。

### 示例代码

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
            users: UnorderedSet::new(StorageKey::Orders), // 这里应该使用 `StorageKey::Users`
        }
    }
}
```

在这个示例中，`orders`和`users`共享相同的id，这可能导致意外的行为。
