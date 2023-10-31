## 未指定 ceil 或 floor 的不当舍入

### 配置

* 检测器 ID：`round`
* 严重程度：中等

### 描述

查找是否在算术运算中使用了舍入。未指定舍入方向的舍入可能会在 DeFi 中被利用。有关更多详细信息，请参阅[如何一次性以 0.000001 BTC 成为百万富翁（neodyme.io）](https://blog.neodyme.io/posts/lending_disclosure/)。

注意：**Rustle** 不会报告开发人员为特定目的实现的舍入函数。

### 示例代码

```rust
let fee = (amount * fee_rate).round();
```

在此示例中，合约开发人员不应使用 `round` 来计算费用。相反，他们应该使用 `ceil` 或 `floor` 来指定舍入方向。
