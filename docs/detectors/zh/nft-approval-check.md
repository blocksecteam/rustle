## 缺乏在NFT转移过程中检查批准ID

### 配置

* 检测器ID：`nft-approval-check`
* 严重程度：高

### 描述

在NFT转移（即`nft_transfer`和`nft_transfer_call`）过程中，如果转移的发送者不是NFT的所有者，则应该检查转移的`approval_id`以确保发送者有权进行此次转移。如果没有进行这个检查，任何人都可以在没有批准的情况下将NFT转移到其他账户。

### 示例代码

在[near-contract-standards](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/core/core_impl.rs#L212)中的代码展示了正确的实现。

```rust
// https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/non_fungible_token/core/core_impl.rs#L215
let app_acc_ids =
    approved_account_ids.as_ref().unwrap_or_else(|| env::panic_str("Unauthorized"));

// 使用批准扩展；获取发送者的approval_id。
let actual_approval_id = app_acc_ids.get(sender_id);

// 如果发送者没有被批准，则触发panic。
if actual_approval_id.is_none() {
    env::panic_str("发送者未被批准");
}

// 如果包含approval_id，则检查是否匹配。
require!(
    approval_id.is_none() || actual_approval_id == approval_id.as_ref(),
    format!(
        "实际的approval_id {:?} 与给定的approval_id {:?} 不同",
        actual_approval_id, approval_id
    )
);
```
