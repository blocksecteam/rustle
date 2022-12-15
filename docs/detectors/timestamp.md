## Dependency of timestamp

### Configuration

* detector id: `timestamp`
* severity: info

### Description

Timestamp dependency may lead to exploitation since it could be manipulated by miners. For example, in a lottery contract, the jackpot is generated using the timestamp, then the miner may have some way to manipulate the timestamp to get the jackpot.

**Rustle** provides this detector to help users find incorrect timestamp uses by locating all uses of `timestamp`.

### Sample code

Here is a sample for a lottery contract. The winner is generated using the timestamp, which means the result can be manipulated by those who can control the timestamp.

```rust
impl Lottery{
    fn get_winner(&self) -> u32 {
        let current_time = env::block_timestamp();
        let winner_id = self.generate_winner(&current_time);

        winner_id
    }
}
```
