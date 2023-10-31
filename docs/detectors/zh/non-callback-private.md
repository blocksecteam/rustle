## 在非回调函数中不正确使用`#[private]`

### 配置

* 检测器ID：`non-callback-private`
* 严重程度：低

### 描述

`#[private]`宏通常用于回调函数中，以确保`current_account_id`等于`predecessor_account_id`。它不应该在非回调函数中使用。否则，一个公共函数将成为一个内部函数。

### 示例代码

```rust
#[near_bindgen]
impl Pool {
    #[private]
    pub fn get_tokens(&self) -> &[AccountId] {
        &self.token_account_ids
    }
}
```

`get_tokens`应该是一个公共接口（一个没有`#[private]`宏修饰的公共函数），供所有人检查池中的所有令牌类型。它不应该被修饰为`#[private]`。
