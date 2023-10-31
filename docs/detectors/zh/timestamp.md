## 时间戳的依赖关系

### 配置

* 检测器ID：`timestamp`
* 严重程度：信息

### 描述

时间戳的依赖关系可能导致被挖矿者操纵。例如，在一个彩票合约中，中奖号码是使用时间戳生成的，那么矿工可能有一些方法来操纵时间戳以获取中奖号码。

**Rustle** 提供了这个检测器来帮助用户找到不正确的时间戳使用，通过定位所有使用 `timestamp` 的地方。

### 示例代码

下面是一个彩票合约的示例。中奖者是使用时间戳生成的，这意味着可以通过控制时间戳的人来操纵结果。

```rust
impl Lottery{
    fn get_winner(&self) -> u32 {
        let current_time = env::block_timestamp();
        let winner_id = self.generate_winner(&current_time);

        winner_id
    }
}
```
