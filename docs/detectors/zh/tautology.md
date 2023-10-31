## 在分支条件中使用重言式

### 配置

* 检测器 ID: `tautology`
* 严重程度: 低

### 描述

查找简单的重言式（条件中涉及`true`或`false`），使得分支变得确定性。

### 示例代码

```rust
let ok: bool = check_state();
if true || ok {
    // ...
} else {
    // ...
}
```

在这个示例中，`else`分支中的代码永远不会被执行。这是因为`true || ok`是一个重言式。
