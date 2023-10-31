## 缺少`#[private]`宏在回调函数中

### 配置

* 检测器ID: `non-private-callback`
* 严重程度: 高

### 描述

回调函数应该使用`#[private]`进行修饰，以确保只有合约本身才能调用它。

如果没有这个宏，任何人都可以调用回调函数，这与原始设计相违背。

### 示例代码

```rust
pub fn callback_stake(&mut self) { // 这是一个回调函数
    // ...
}
```

函数`callback_stake`应该使用`#[private]`进行修饰:

```rust
#[private]
pub fn callback_stake(&mut self) { // 这是一个回调函数
    // ...
}
```

有了这个宏，只有合约本身才能调用`callback_stake`。
