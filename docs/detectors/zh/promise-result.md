## Promise结果

### 配置

* 检测器ID: `promise-result`
* 严重程度: 信息

### 描述

查找所有使用`env::promise_result`的地方，该函数提供了Promise执行的结果。

该检测器有助于快速定位处理Promise结果的逻辑。

### 示例代码

```rust
// https://github.com/ref-finance/ref-contracts/blob/b33c3a488b18f9cff82a3fdd53bf65d6aac09e15/ref-exchange/src/lib.rs#L434
let cross_call_result = match env::promise_result(0) {
    PromiseResult::Successful(result) => result,
    _ => env::panic(ERR124_CROSS_CALL_FAILED.as_bytes()),
};
```

在这个示例中，根据Promise结果的状态（成功或失败）来处理Promise结果。
