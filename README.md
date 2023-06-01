# Rustle

<img src="./logo.png" alt="Rustle" width="500"/>

[![CI Status](https://img.shields.io/github/actions/workflow/status/blocksecteam/rustle/ci.yml?branch=main&label=ci)](https://github.com/blocksecteam/rustle/actions/workflows/ci.yml)
[![Build-Image Status](https://img.shields.io/github/actions/workflow/status/blocksecteam/rustle/build-image.yml?branch=main&label=build-image)](https://github.com/blocksecteam/rustle/actions/workflows/build-image.yml)
[![License: AGPL v3](https://img.shields.io/github/license/blocksecteam/rustle)](LICENSE)
[![AwesomeNEAR](https://img.shields.io/badge/Project-AwesomeNEAR-054db4)](https://awesomenear.com/rustle)
[![Devpost](https://img.shields.io/badge/Honorable%20Mention-Devpost-003e54)](https://devpost.com/software/rustle)

Rustle is an automatic static analyzer for NEAR smart contracts in Rust. It can help to locate tens of different vulnerabilities in NEAR smart contracts.
According to [DefiLlama](https://defillama.com/chain/Near), among the top 10 DApps in NEAR, 8 are audited by BlockSec. With rich audit experience and a deep understanding of NEAR protocol, we build this tool and share it with the community.

## Get started

### Prerequisite

#### Linux setup

Install the required toolkits with the following commands for **Rustle** in Linux. Commands are tested in Ubuntu 20.04 LTS.

```bash
# install Rust Toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# install LLVM 15
sudo bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)" 15

# install Python toolchain
sudo apt install python3 python3-pip    # requires python >= 3.8
pip3 install -r utils/requirements.txt  # you need to clone this repo first

# add WASM target
rustup target add wasm32-unknown-unknown

# install other components
sudo apt install figlet
cargo install rustfilt

# [optional] useful tools for developing
LLVM_VERSION=
sudo apt install clangd-$LLVM_VERSION clang-format-$LLVM_VERSION clang-tidy-$LLVM_VERSION
```

#### macOS setup

The following commands are for users using macOS, they are tested only on Apple Silicon Mac, so use them with caution.

```bash
# install Rust Toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# install LLVM 15
brew install llvm@15

# install Python packages
pip3 install -r utils/requirements.txt  # you need to clone this repo first
                                        # using macOS default python3

# add WASM target
rustup target add wasm32-unknown-unknown

# install other components
brew install figlet coreutils gsed
cargo install rustfilt
```

#### Docker

We provide a docker solution.

```bash
# build the image
docker build --build-arg UID=`id -u` --build-arg GID=`id -g` -t rustle .

# run a container from the image
docker run --name rustle -it -v `pwd`:/rustle -w /rustle rustle bash

# exec the container
docker start rustle
docker exec -it -w /rustle rustle bash
```

### Usage

```bash
./rustle [-t|--tg_dir <tg_dir>] [-d|--detector <detector_list>] [-o|--output <output_dir>] [-h|--help] <src_dir>
```

* `src_dir`: Path to the contract source.
* `tg_dir`: Path to the contract build target. Defaults to be same as `src_dir`.
* `detector`: The detector list. It can be used to pass multiple *detectors* or *groups* separated by `,`. Defaults to `all`.
    * pass `all` *group* to enable all detectors.
    * pass `high`, `medium`, `low` and `info` *groups* to enable detector groups with different severity (refer to [Detectors](#detectors))
    * pass `nep-ft`, `nep-storage` and `nep-nft` *groups* to enable detectors implemented for specified NEP (refer to [NEP detector groups](#nep-detector-groups))
    * pass *detector ids* in the [table](#detectors) to enable those detectors
* `output`: Path where audit reports will be generated in. Defaults to `./audit-result`.

Note: if the target bit code (`.bc` binary) built by cargo is not in the `$src_dir`, use `-t|--tg_dir` to set the target's directory, or it will be set to `$src_dir` by default.

The command below shows an example of analyzing the LiNEAR.

```bash
# clone LiNEAR
git clone https://github.com/linear-protocol/LiNEAR.git ~/near-repo/LiNEAR

# run Rustle
./rustle -t ~/near-repo/LiNEAR ~/near-repo/LiNEAR/contracts/linear

# [optional] run Rustle on specified detectors or severity groups and save audit reports in `~/linear-report`
./rustle -t ~/near-repo/LiNEAR ~/near-repo/LiNEAR/contracts/linear -d high,medium,complex-loop -o ~/linear-report
```

A CSV-format report will be generated in the directory "./audit-result".

## Detectors

All vulnerabilities **Rustle** can find.

| Detector ID             | Description                                                                                 | Severity |
| ----------------------- | ------------------------------------------------------------------------------------------- | -------- |
| `unhandled-promise`     | [find `Promises` that are not handled](docs/detectors/unhandled-promise.md)                 | High     |
| `non-private-callback`  | [missing macro `#[private]` for callback functions](docs/detectors/non-private-callback.md) | High     |
| `reentrancy`            | [find functions that are vulnerable to reentrancy attack](docs/detectors/reentrancy.md)     | High     |
| `unsafe-math`           | [lack of overflow check for arithmetic operation](docs/detectors/unsafe-math.md)            | High     |
| `self-transfer`         | [missing check of `sender != receiver`](docs/detectors/self-transfer.md)                    | High     |
| `incorrect-json-type`   | [incorrect type used in parameters or return values](docs/detectors/incorrect-json-type.md) | High     |
| `unsaved-changes`       | [changes to collections are not saved](docs/detectors/unsaved-changes.md)                   | High     |
| `nft-approval-check`    | [find `nft_transfer` without check of `approval id`](docs/detectors/nft-approval-check.md)  | High     |
| `nft-owner-check`       | [find approve or revoke functions without owner check](docs/detectors/nft-owner-check.md)   | High     |
| `div-before-mul`        | [precision loss due to incorrect operation order](docs/detectors/div-before-mul.md)         | Medium   |
| `round`                 | [rounding without specifying ceil or floor](docs/detectors/round.md)                        | Medium   |
| `lock-callback`         | [panic in callback function may lock contract](docs/detectors/lock-callback.md)             | Medium   |
| `yocto-attach`          | [no `assert_one_yocto` in privileged function](docs/detectors/yocto-attach.md)              | Medium   |
| `dup-collection-id`     | [duplicate id uses in collections](docs/detectors/dup-collection-id.md)                     | Medium   |
| `unregistered-receiver` | [no panic on unregistered transfer receivers](docs/detectors/unregistered-receiver.md)      | Medium   |
| `nep${id}-interface`    | [find all unimplemented NEP interface](docs/detectors/nep-interface.md)                     | Medium   |
| `prepaid-gas`           | [missing check of prepaid gas in `ft_transfer_call`](docs/detectors/prepaid-gas.md)         | Low      |
| `non-callback-private`  | [macro `#[private]` used in non-callback function](docs/detectors/non-callback-private.md)  | Low      |
| `unused-ret`            | [function result not used or checked](docs/detectors/unused-ret.md)                         | Low      |
| `upgrade-func`          | [no upgrade function in contract](docs/detectors/upgrade-func.md)                           | Low      |
| `tautology`             | [tautology used in conditional branch](docs/detectors/tautology.md)                         | Low      |
| `storage-gas`           | [missing balance check for storage expansion](docs/detectors/storage-gas.md)                | Low      |
| `unclaimed-storage-fee` | [missing balance check before storage unregister](docs/detectors/unclaimed-storage-fee.md)  | Low      |
| `inconsistency`         | [use of similar but slightly different symbol](docs/detectors/inconsistency.md)             | Info     |
| `timestamp`             | [find all uses of `timestamp`](docs/detectors/timestamp.md)                                 | Info     |
| `complex-loop`          | [find all loops with complex logic which may lead to DoS](docs/detectors/complex-loop.md)   | Info     |
| `ext-call`              | [find all cross-contract invocations](docs/detectors/ext-call.md)                           | Info     |
| `promise-result`        | [find all uses of promise result](docs/detectors/promise-result.md)                         | Info     |
| `transfer`              | [find all transfer actions](docs/detectors/transfer.md)                                     | Info     |
| `public-interface`      | [find all public interfaces](docs/detectors/public-interface.md)                            | Info     |

### NEP detector groups

Apart from the groups by severity level, **Rustle** provides some detector groups by corresponding NEP. Currently, **Rustle** supports the following groups.

[nep141]: https://github.com/near/NEPs/blob/master/neps/nep-0141.md
[nep145]: https://github.com/near/NEPs/blob/master/neps/nep-0145.md
[nep171]: https://github.com/near/NEPs/blob/master/neps/nep-0171.md
[nep178]: https://github.com/near/NEPs/blob/master/neps/nep-0178.md

| NEP                                  | Detector Group ID | Detector IDs                                                 |
| ------------------------------------ | ----------------- | ------------------------------------------------------------ |
| [NEP-141][nep141]                    | `nep-ft`          | `nep141-interface`, `self-transfer`, `unregistered-receiver` |
| [NEP-145][nep145]                    | `nep-storage`     | `nep145-interface`, `unclaimed-storage-fee`                  |
| [NEP-171][nep171], [NEP-178][nep178] | `nep-nft`         | `nep171-interface`, `nft-approval-check`, `nft-owner-check`  |

## Add new detectors

1. Fork this repo to your account.
2. Put the new detector under [/detectors](/detectors/).
3. Add a detection target in [/Makefile](/Makefile) with commands to run your detector.
4. Add the target to the dependency of `audit` target and its name to [detector list](/rustle#L146) and [severity groups](/rustle#L169) in `./rustle` script.
5. Add processing code in [utils/audit.py](/utils/audit.py) (refer to other detectors' code in `audit.py`).
6. Submit a pull request from your branch to the main.

## Note

**Rustle** can be used in the development process to scan the NEAR smart contracts iteratively. This can save a lot of manual effort and mitigate part of potential issues. However, vulnerabilities in complex logic or related to semantics are still the limitation of **Rustle**. Locating complicated semantic issues requires the experts in [BlockSec](https://blocksec.com/) to conduct exhaustive and thorough reviews. [Contact us](audit@blocksec.com) for audit service.

## License

This project is under the AGPLv3 License. See the [LICENSE](LICENSE) file for the full license text.
