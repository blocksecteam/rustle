## 检查 NEP 接口的完整性

### 配置

* 检测器 id: `nep${id}-interface` (`${id}` 是 NEP ID)
* 严重性: 中等

### 描述

[NEPs](https://github.com/near/NEPs) 代表 Near Enhancement Proposals，这些是对 NEAR 协议规范和标准的一些更改。目前，有关于 FT、NFT、MT 和存储的 NEPs，列在一个[表格](https://github.com/near/NEPs#neps)中。

要使用这个检测器，你需要在检测器 id 中指定 NEP id，例如：

```bash
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep141-interface  # 可替代令牌标准
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep145-interface  # 存储管理
./rustle ~/Git/near-sdk-rs/near-contract-standards -t ~/Git/near-sdk-rs -d nep171-interface  # 不可替代令牌标准
```

### 示例代码

检测器 `nep-interface` 的[示例](/examples/nep-interface/)是从 near-contract-standards 中改编的。
