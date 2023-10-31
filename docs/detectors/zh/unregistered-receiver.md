## NEP-141转账在未注册的接收方账户上不会引发panic

### 配置

* 检测器ID：`unregistered-receiver`
* 严重程度：中等

### 描述

根据[NEP-141](https://github.com/near/NEPs/blob/master/neps/nep-0141.md)的[实现](https://github.com/near/near-sdk-rs/blob/63ba6ecc9439ec1c319c1094d581653698229473/near-contract-standards/src/fungible_token/core_impl.rs#L58)，未注册的转账接收方应该导致panic。

开发人员可能会尝试为接收方注册一个新账户，而不要求支付存储费用，这可能导致DoS攻击。账户注册的一种可能实现在[storage_impl.rs](https://github.com/near/near-sdk-rs/blob/1859ce4c201d2a85fbe921fdada1df59b00d2d8c/near-contract-standards/src/fungible_token/storage_impl.rs#L45)中。

### 示例代码

下面的代码段展示了两种解包（unwrap）账户余额（即接收方余额）的方式。安全的方式在账户未注册时会引发panic，而不安全的方式会返回默认余额0而不引发panic。在不安全的情况下，将插入一个`AccountId == account_id`的新键到`accounts`映射中，其存储费用由`Contract`的赞助商支付。

有关完整的示例代码，请参考[examples/unregistered-receiver](/examples/unregistered-receiver)。

```rust
#[near_bindgen]
#[derive(BorshDeserialize, BorshSerialize)]
pub struct Contract {
    accounts: UnorderedMap<AccountId, Balance>,
    total_supply: Balance,
}

impl Contract {
    pub fn internal_unwrap_balance_safe(&self, account_id: &AccountId) -> Balance {
        self.accounts
            .get(account_id)
            .unwrap_or_else(|| env::panic_str("Account is not registered"))
    }
    pub fn internal_unwrap_balance_unsafe(&self, account_id: &AccountId) -> Balance {
        self.accounts.get(account_id).unwrap_or(0)
    }
}
```
