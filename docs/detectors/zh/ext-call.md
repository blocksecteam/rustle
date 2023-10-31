## 跨合约调用

### 配置

* 检测器 ID: `ext-call`
* 严重程度: 信息

### 描述

查找所有的跨合约调用。

我们提供这个检测器来帮助用户快速定位所有的跨合约调用，以便进行进一步的分析。

### 示例代码

```rust
// https://docs.near.org/sdk/rust/cross-contract/callbacks#calculator-example
#[ext_contract(ext_calculator)]
trait Calculator {
    fn sum(&self, a: U128, b: U128) -> U128;
}

#[near_bindgen]
impl Contract {
    pub fn sum_a_b(&mut self, a: U128, b: U128) -> Promise {
        let calculator_account_id: AccountId = "calc.near".parse().unwrap();
        ext_calculator::ext(calculator_account_id).sum(a, b)  // 在远程上运行 sum(a, b)
    }
}
```

> 宏 `#[ext_contract(...)]` 将一个 Rust trait 转换为一个具有静态方法的模块，允许用户进行跨合约调用。这些静态方法中的每一个都接受由 Trait 定义的位置参数，然后是 `receiver_id`、附加的存款和燃料数量，并返回一个新的 `Promise`。

在这个示例中，合约可以通过跨合约调用（即 `ext_calculator::ext(calculator_account_id).sum(a, b)`）获取 `a` 和 `b` 的和。
