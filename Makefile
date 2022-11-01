.PHONY: pass analysis echo tg_ir audit audit-report \
	transfer div-before-mul shared-var-get-shared shared-var-get-invoke unsafe-math reentrancy round variable struct-member admin-func public-func tautology lock-callback non-callback-private non-private-callback incorrect-json-type complex-loop \
	clean clean_pass clean_demo clean_tg clean_tmp

SHELL := /bin/bash # Use bash syntax

export
# Config Env
TOP = $(shell pwd)
LLVM_DIR = /usr/lib/llvm-14

# Binaries
LLVM_CONFIG = ${LLVM_DIR}/bin/llvm-config
LLVM_CLANG  = ${LLVM_DIR}/bin/clang
LLVM_OPT    = ${LLVM_DIR}/bin/opt

# Flags
INCLUDE = -I${TOP}/detectors
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags) ${INCLUDE} -Wl,-znodelete -fno-rtti -fPIC -shared # -std=c++11  -Ofast
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

OPTFLAGS = -enable-new-pm=0

ifndef NEAR_TG_DIR
	NEAR_TG_DIR=${NEAR_SRC_DIR}
endif

ifndef TMP_DIR
	TMP_DIR=${TOP}/.tmp/
endif

TG_MANIFESTS = $(shell find ${NEAR_SRC_DIR} -name "Cargo.toml" -not -path "**/node_modules/*")

echo:
	@echo "NEAR_SRC_DIR = ${NEAR_SRC_DIR}"
	@echo "NEAR_TG_DIR  = ${NEAR_TG_DIR}"
	@echo "TG_MANIFESTS = ${TG_MANIFESTS}"

pass:
	make -C detectors pass -j16

tg_ir:
	-@for i in ${TG_MANIFESTS} ; do \
		cargo rustc --target wasm32-unknown-unknown --manifest-path $$i -- -Awarnings --emit=llvm-ir,llvm-bc ; \
	done
	@mkdir -p ${TMP_DIR}

analysis: unsafe-math round reentrancy div-before-mul transfer timestamp promise-result upgrade-func self-transfer prepaid-gas unhandled-promise yocto-attach complex-loop \
	tautology unused-ret inconsistency lock-callback non-callback-private non-private-callback incorrect-json-type

callback: tg_ir
	@rm -f ${TMP_DIR}/.callback.tmp
	@make -C detectors callback.so
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/callback.so -callback {} -o /dev/null

ext-call-trait: tg_ir
	@rm -f ${TMP_DIR}/.ext-call.tmp
	@make -C detectors ext_call_trait.so
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/ext_call_trait.so -ext-call-trait {} -o /dev/null

ext-call: tg_ir
	@make -C detectors ext_call.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/ext_call.so -ext-call {} -o /dev/null

complex-loop: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors complex_loop.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/complex_loop.so -complex-loop {} -o /dev/null

unsafe-math: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp ${TMP_DIR}/.$@-toml.tmp
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@python3 ./detectors/unsafe-math-toml.py ${NEAR_SRC_DIR}
	@make -C detectors unsafe_math.so
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/unsafe_math.so -unsafe-math {} -o /dev/null

round: tg_ir
	@make -C detectors round.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/round.so -round {} -o /dev/null

struct-member: tg_ir find-struct
	@make -C detectors struct_member.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/struct_member.so -struct-member {} -o /dev/null

reentrancy: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors reentrancy.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/reentrancy.so -reentrancy {} -o /dev/null

div-before-mul: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors div_before_mul.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/div_before_mul.so -div-before-mul {} -o /dev/null

transfer: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors transfer.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/transfer.so -transfer {} -o /dev/null

timestamp: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors timestamp.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/timestamp.so -timestamp {} -o /dev/null

promise-result: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors promise_result.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/promise_result.so -promise-result {} -o /dev/null

upgrade-func: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors upgrade_func.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/upgrade_func.so -upgrade-func {} -o /dev/null
	@test -s .tmp/.upgrade-func.tmp || echo -e "\e[33m[!] Upgrade func not found\e[0m"

self-transfer: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors self_transfer.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/self_transfer.so -self-transfer {} -o /dev/null

prepaid-gas: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors prepaid_gas.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/prepaid_gas.so -prepaid-gas {} -o /dev/null

all-call: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors all_call.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/all_call.so -all-call {} -o /dev/null

unhandled-promise: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors unhandled_promise.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/unhandled_promise.so -unhandled-promise {} -o /dev/null

yocto-attach: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors yocto_attach.so
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@find ${NEAR_TG_DIR}// -name '*.bc' | xargs -i $(LLVM_OPT) ${OPTFLAGS} -load detectors/yocto_attach.so -yocto-attach {} -o /dev/null

tautology:
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.bc' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi
	@python3 ./detectors/tautology.py ${NEAR_SRC_DIR}

unused-ret: all-call
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/unused-ret.py ${NEAR_SRC_DIR}

inconsistency:
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/inconsistency.py ${NEAR_SRC_DIR}

lock-callback: callback
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/lock-callback.py ${NEAR_SRC_DIR}

non-callback-private: callback
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/non-callback-private.py ${NEAR_SRC_DIR}

non-private-callback: callback
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/non-private-callback.py ${NEAR_SRC_DIR}

incorrect-json-type: find-struct
	@if test $(shell find ${NEAR_TG_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ -w 200 ; \
	fi
	@python3 ./detectors/incorrect-json-type.py ${NEAR_SRC_DIR}

find-struct:  # provide .struct.tmp and .struct-member.tmp
	@python3 ./utils/findStruct.py ${NEAR_SRC_DIR}

audit: promise-result reentrancy transfer timestamp div-before-mul unsafe-math round find-struct upgrade-func self-transfer prepaid-gas unhandled-promise yocto-attach complex-loop \
	tautology unused-ret inconsistency lock-callback non-callback-private non-private-callback incorrect-json-type
	@python3 ./utils/audit.py ${NEAR_SRC_DIR}

audit-report:
	@python3 ./utils/audit.py ${NEAR_SRC_DIR}

clean: clean_pass clean_tg
clean_pass:
	make -C detectors clean

clean_tg:
	@for i in ${TG_MANIFESTS} ; do \
		cargo clean --manifest-path=$$i ; \
	done

clean_tmp:
	rm -rf ${TMP_DIR}

compile_commands.json: clean_pass
	if [[ $(shell bear --version | cut -d' ' -f2) = 2.4.* ]] ; then \
		bear make -C detectors pass ; \
	else \
		bear -- make -C detectors pass ; \
	fi

