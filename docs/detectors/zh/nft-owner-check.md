## 在 NFT 转移过程中缺少检查批准 id

### 配置

* 检测器 id：`nft-owner-check`
* 严重性：高

### 描述

在 [NEP-178](https://github.com/near/NEPs/blob/master/neps/nep-0178.md) 的设计中，NFT 的所有者可以使用指定的接口（即，`nft_approve`，`nft_revoke` 和 `nft_revoke_all`）来批准或撤销批准。这些接口应实现所有者检查，以确保只有所有者才能调用它们，否则，任何人都可以修改 NFT 的批准。

### 示例代码

在 [near-contract-standards](https://github.com/near/near-sdk-rs/blob/a903f8c44a7be363d960838d92afdb22d1ce8b87/near-contract-standards/src/non_fungible_token/approval/approval_impl.rs) 中的代码显示了正确的实现。

```rust
// 应为 `nft_approve`，`nft_revoke` 和 `nft_revoke_all` 实现
let owner_id = expect_token_found(self.owner_by_id.get(&token_id));
let predecessor_account_id = env::predecessor_account_id();

require!(predecessor_account_id == owner_id, "Predecessor must be token owner.");
```
