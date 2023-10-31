## 没有溢出检查的不安全数学运算

### 配置

* 检测器 ID：`unsafe-math`
* 严重程度：高

### 描述

对所有算术运算启用溢出检查。否则，可能会发生溢出，导致结果不正确。

NEAR 合约中的溢出检查可以通过两种不同的方法实现。

1. \[推荐\] 在 cargo 清单中打开 `overflow-checks`。在这种情况下，可以使用 `+`、`-` 和 `*` 进行算术运算。
2. 使用安全的数学函数（例如 `checked_xxx()`）进行算术运算。

### 示例代码

在这个例子中，由于在 cargo 清单中关闭了 `overflow-checks` 标志，使用 `+` 可能会导致溢出。

```toml
[profile.xxx]  # `xxx` 等于 `dev` 或 `release`
overflow-checks = false
```

```rust
let a = b + c;
```

建议使用以下代码进行带有 `overflow-checks=false` 的加法运算

```rust
let a = b.checked_add(c);
```
