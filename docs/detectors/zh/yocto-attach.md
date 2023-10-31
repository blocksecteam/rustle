## 缺乏双因素认证

### 配置

* 检测器 ID：`yocto-attach`
* 严重程度：中等

### 描述

特权函数应该检查是否已连接一个 yocto NEAR。这将为安全考虑在 NEAR 钱包中启用双因素认证。

可以通过在合约中添加 `assert_one_yocto` 来实现此功能，这是所有特权函数的推荐做法。

注意：由于缺乏语义信息，很难确定所有特权函数。目前，**Rustle** 将所有涉及 `predecessor_account_id` 比较的函数视为特权函数。

### 示例代码

```rust
pub(crate) fn assert_owner(&self) {
    assert_eq!(
        env::predecessor_account_id(),
        self.owner_id,
        "{}", ERR100_NOT_ALLOWED
    );
}

// https://github.com/ref-finance/ref-contracts/blob/536a60c842e018a535b478c874c747bde82390dd/ref-exchange/src/owner.rs#L16
#[payable]
pub fn set_owner(&mut self, owner_id: ValidAccountId) {
    assert_one_yocto();
    self.assert_owner();
    self.owner_id = owner_id.as_ref().clone();
}
```

在这个示例中，`set_owner` 是一个特权函数，只能由合约所有者调用。函数 `assert_owner` 通过检查其 `predecessor_account_id` 来确保特权。为了安全起见，这个函数应该有一个双因素认证，可以通过在函数中添加 `assert_one_yocto` 调用来实现。
