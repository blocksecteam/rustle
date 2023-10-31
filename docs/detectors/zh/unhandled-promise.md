## 未处理的 Promise 结果

### 配置

* 检测器 ID：`unhandled-promise`
* 严重程度：高

### 描述

Promise 的结果应该始终由回调函数或另一个 Promise 处理。不建议在合约中留下未处理的 Promise。否则，如果 Promise 失败，状态的更改将无法回滚。

### 示例代码

```rust
token.ft_transfer_call(receiver, U128(amount), None, "".to_string());
```

在这个示例中，合约调用了 `ft_transfer_call`，但没有使用指定的回调函数（例如 `resolve_transfer`）处理其 Promise 结果。因此，合约将无法知道转账是否成功。如果转账失败，合约状态将无法回滚。
