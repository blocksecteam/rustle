## 不一致的符号

### 配置

* 检测器ID：`inconsistency`
* 严重程度：信息

### 描述

具有类似但稍有不同名称的符号可能会被错误使用。这种错误很难手动发现，但可能导致严重的结果。

### 示例代码

```rust
ext::ft_transfer(
    receiver_id,
    U128(amount),
    None,
    token_id,
    1,
    GAS_FOR_NFT_TRANSFER  // 这里应该使用GAS_FOR_FT_TRANSFER
)
```

在这个示例中，合约应该使用`GAS_FOR_FT_TRANSFER`而不是`GAS_FOR_NFT_TRANSFER`。这两个常量的值可能有很大的差异，而对于`ft_transfer`的gas可能是不足的。
