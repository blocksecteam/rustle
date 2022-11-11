# Rustle

<img src="./logo.png" alt="Rustle" width="500"/>

[![Unit-Test Status](https://img.shields.io/github/workflow/status/blocksecteam/rustle/unit-test?label=unit-test)](https://github.com/blocksecteam/rustle/actions/workflows/unit-test.yml)
[![Build-Image Status](https://img.shields.io/github/workflow/status/blocksecteam/rustle/build-image?label=build-image)](https://github.com/blocksecteam/rustle/actions/workflows/build-image.yml)
[![License: AGPL v3](https://img.shields.io/github/license/blocksecteam/rustle)](LICENSE)
[![AwesomeNEAR](https://img.shields.io/badge/AwesomeNEAR-Rustle-blue)](https://awesomenear.com/rustle)

Rustle is an automatic static analyzer for NEAR smart contracts in Rust. It can help to locate tens of different vulnerabilities in NEAR smart contracts. 
According to [DefiLlama](https://defillama.com/chain/Near), among the top 10 DApps in NEAR, 8 are audited by BlockSec. With rich audit experience and a deep understanding of NEAR protocol, we build this tool and share it with the community.

## Get started

### Prerequisite

Install the required toolkits with the following commands for **Rustle**. Commands are tested in Ubuntu 20.04 LTS.

```bash
# install Rust Toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# install LLVM 15
sudo bash -c "$(wget -O - https://apt.llvm.org/llvm.sh)" 15

# install Python toolchain
sudo apt install python3 python3-pip python3-tk   
pip3 install -r utils/requirements.txt  # you need to clone this repo first

# add WASM target
rustup target add wasm32-unknown-unknown

# install other components
sudo apt install libudev-dev figlet
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
    * pass `high`, `medium`, `low` and `info` *groups* to enable detector groups with different severity
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

| Detector Id            | Description                                                                                 | Severity |
| ---------------------- | ------------------------------------------------------------------------------------------- | -------- |
| `unhandled-promise`    | [find `Promises` that are not handled](docs/detectors/unhandled-promise.md)                 | High     |
| `non-private-callback` | [missing macro `#[private]` for callback functions](docs/detectors/non-private-callback.md) | High     |
| `reentrancy`           | [find functions that are vulnerable to reentrancy attack](docs/detectors/reentrancy.md)     | High     |
| `unsafe-math`          | [lack of overflow check for arithmetic operation](docs/detectors/unsafe-math.md)            | High     |
| `self-transfer`        | [missing check of `sender != receiver`](docs/detectors/self-transfer.md)                    | High     |
| `incorrect-json-type`  | [incorrect type used in parameters or return values](docs/detectors/incorrect-json-type.md) | High     |
| `div-before-mul`       | [precision loss due to incorrect operation order](docs/detectors/div-before-mul.md)         | Medium   |
| `round`                | [rounding without specifying ceil or floor](docs/detectors/round.md)                        | Medium   |
| `lock-callback`        | [panic in callback function may lock contract](docs/detectors/lock-callback.md)             | Medium   |
| `yocto-attach`         | [no `assert_one_yocto` in privileged function](docs/detectors/yocto-attach.md)              | Medium   |
| `prepaid-gas`          | [missing check of prepaid gas in `ft_transfer_call`](docs/detectors/prepaid-gas.md)         | Low      |
| `non-callback-private` | [macro `#[private]` used in non-callback function](docs/detectors/non-callback-private.md)  | Low      |
| `unused-ret`           | [function result not used or checked](docs/detectors/unused-ret.md)                         | Low      |
| `upgrade-func`         | [no upgrade function in contract](docs/detectors/upgrade-func.md)                           | Low      |
| `tautology`            | [tautology used in conditional branch](docs/detectors/tautology.md)                         | Low      |
| `inconsistency`        | [use of similar but slightly different symbol](docs/detectors/inconsistency.md)             | Low      |
| `timestamp`            | [find all uses of `timestamp`](docs/detectors/timestamp.md)                                 | Info     |
| `complex-loop`         | [find all loops with complex logic which may lead to DoS](docs/detectors/complex-loop.md)   | Info     |
| `ext-call`             | [find all cross-contract invocations](docs/detectors/ext-call.md)                           | Info     |
| `promise-result`       | [find all uses of promise result](docs/detectors/promise-result.md)                         | Info     |
| `transfer`             | [find all transfer actions](docs/detectors/transfer.md)                                     | Info     |
| `public-interface`     | [find all public interfaces](docs/detectors/public-interface.md)                            | Info     |

## Add new detector

1. Fork this repo to your account.
2. Put the new detector under [/detectors](/detectors/) (for the LLVM detector written in C++, add a build target in [detectors/Makefile](/detectors/Makefile)).
3. Add a detection target in [/Makefile](/Makefile) with commands to run your detector.
4. Add the target to the dependency of `audit` target and its name to [detector list](/rustle#L146) and [severity groups](/rustle#L169) in `./rustle` script.
5. Add processing code in [utils/audit.py](/utils/audit.py) (refer to other detectors' code in `audit.py`).
6. Submit a pull request from your branch to the main.

## Note

**Rustle** can be used in the development process to scan the NEAR smart contracts iteratively. This can save a lot of manual effort and mitigate part of potential issues. However, vulnerabilities in complex logic or related to semantics are still the limitation of **Rustle**. Locating complicated semantic issues requires the experts in [BlockSec](https://blocksec.com/) to conduct exhaustive and thorough reviews. [Contact us](audit@blocksec.com) for audit service.

## License

This project is under the AGPLv3 License. See the [LICENSE](LICENSE) file for the full license text.
