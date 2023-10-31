## 查找所有公共接口

### 配置

* 检测器ID：`public-interface`
* 严重程度：信息

### 描述

公共接口是可以被其他函数调用的函数。具体来说，它们是`#[near_bindgen]`结构体中没有`#[private]`宏修饰的公共函数。

### 示例代码

```rust
#[near_bindgen]
impl Contract {
    pub fn withdraw(&mut self, amount: U128) -> Promise {
        unimplemented!()
    }

    pub fn check_balance(&self) -> U128 {
        unimplemented!()
    }

    #[private]
    pub fn callback_withdraw(&mut self, amount: U128) {
        unimplemented!()
    }

    fn sub_balance(&mut self, amount: u128) {
        unimplemented!()
    }
}
```

在这个示例代码中，所有四个函数都是`#[near_bindgen]`结构体`Contract`的方法。但只有`withdraw`和`check_balance`是公共接口。`callback_withdraw`是一个被`#[private]`修饰的公共函数，因此它是一个内部函数。`sub_balance`是`Contract`的私有函数。
