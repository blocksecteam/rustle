## 使用不正确的 JSON 类型

### 配置

* 检测器 ID：`incorrect-json-type`
* 严重程度：高

### 描述

不要将类型 `i64`、`i128`、`u64` 或 `u128` 用作公共接口（`#[near_bindgen]` 结构体中没有 `#[private]` 宏的公共函数）的参数或返回值。

这是因为 JSON 支持的最大整数是 2\^53-1。

请查看 https://2ality.com/2012/04/number-encoding.html 了解更多详情。

在 Near SDK 中，建议开发者使用类型 `I64`、`I128`、`U64` 和 `U128`。

### 示例代码

```rust
// https://github.com/ref-finance/ref-contracts/blob/c580d8742d80033a630a393180163ab70f9f3c94/ref-exchange/src/views.rs#L15
pub struct ContractMetadata {
    pub version: String,
    pub owner: AccountId,
    pub guardians: Vec<AccountId>,
    pub pool_count: u64,  // 应该使用 `U64`
    pub state: RunningState,
    pub exchange_fee: u32,
    pub referral_fee: u32,
}

// https://github.com/ref-finance/ref-contracts/blob/c580d8742d80033a630a393180163ab70f9f3c94/ref-exchange/src/views.rs#L171
#[near_bindgen]
impl Contract {
    pub fn metadata(&self) -> ContractMetadata {  // 返回类型中的 `u64`
        // ...
    }
}
```

在这个示例中，`metadata` 使用了结构体 `ContractMetadata`，其中包含返回值中的 `pool_count: u64`。由于公共函数的返回值是通过 JSON 发送回前端的，而 `pool_count` 的值可能超过 2\^53-1。因此，应该使用 Near SDK 中的字符串类型 `U64`。
