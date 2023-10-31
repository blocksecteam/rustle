## 集合的更改未保存

### 配置

* 检测器 ID：`unsaved-changes`
* 严重程度：高

### 描述

[NEAR SDK](https://crates.io/crates/near-sdk) 提供了一些可以存储键值映射的映射集合。您可以使用 `get` 方法通过键获取值，并通过调用 `insert` 方法插入或覆盖指定键的值。集合使用 borsh 进行序列化和反序列化，当您想对集合进行一些更改时，需要先通过 `get` 方法获取现有值，然后对其进行更改，然后再次将其 `insert` 到集合中。如果没有调用 `insert` 函数，更改将不会保存到合约中。

请注意，自 NEAR SDK 的 [v4.1.0](https://docs.rs/near-sdk/4.1.0/near_sdk/index.html) 版本以来，开发人员可以在 [`near_sdk::store`](https://docs.rs/near-sdk/4.1.0/near_sdk/store/index.html) 中使用映射，它们将自动将更改写回以避免 `near_sdk::collections` 引入的此问题。

### 示例代码

```rust
#[allow(unused)]
pub fn modify(&mut self, change: I128) {
    let mut balance = self
        .accounts
        .get(&env::predecessor_account_id())
        .unwrap_or_else(|| env::panic_str("Account is not registered"));

    balance = balance
        .checked_add(change.0)
        .unwrap_or_else(|| env::panic_str("Overflow"));

    // self.accounts
    //     .insert(&env::predecessor_account_id(), &balance);
}
```

在此示例中，`balance` 是从 `self.accounts` 中通过调用者的账户 ID 索引的值，合约通过调用 `checked_add` 对其进行了一些更改。然而，如果没有调用 `insert` 函数，更改将不会保存。
