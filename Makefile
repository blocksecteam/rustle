.PHONY: pass analysis echo tg_ir audit audit-report \
	transfer div-before-mul shared-var-get-shared shared-var-get-invoke unsafe-math reentrancy round variable struct-member admin-func public-func tautology lock-callback non-callback-private non-private-callback incorrect-json-type complex-loop \
	clean clean_pass clean_demo clean_tg clean_tmp lint lint-diff lint-fix

# SHELL := /bin/bash # Use bash syntax

export
# Config Env
TOP = $(shell pwd)

ifeq ($(shell uname -s), Linux)
	LLVM_DIR = $(shell llvm-config-15 --obj-root)
else ifeq ($(shell uname -s), Darwin)
	LLVM_DIR = $(shell brew --prefix llvm@15)
else
	LLVM_DIR = $(shell llvm-config --obj-root)
endif

# Binaries
LLVM_CONFIG = ${LLVM_DIR}/bin/llvm-config
LLVM_CLANG  = ${LLVM_DIR}/bin/clang
LLVM_OPT    = ${LLVM_DIR}/bin/opt

# Flags
INCLUDE = -I${TOP}/detectors
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags) ${INCLUDE} -fno-rtti -fPIC
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags) -shared

OPTFLAGS = -enable-new-pm=0

ifeq ($(shell uname -s), Darwin)
	LDFLAGS += -undefined dynamic_lookup
endif


ifndef NEAR_TG_DIR
	NEAR_TG_DIR=${NEAR_SRC_DIR}
endif

ifndef TMP_DIR
	TMP_DIR=${TOP}/.tmp/
endif

export TG_MANIFESTS = $(shell find ${NEAR_SRC_DIR} -name "Cargo.toml" -not -path "**/node_modules/*")


echo:
	@echo "NEAR_SRC_DIR = ${NEAR_SRC_DIR}"
	@echo "NEAR_TG_DIR  = ${NEAR_TG_DIR}"
	@echo "TG_MANIFESTS = ${TG_MANIFESTS}"

pass:
	make -C detectors pass

tg_ir:
	-@for i in ${TG_MANIFESTS} ; do \
		cargo rustc --target wasm32-unknown-unknown --manifest-path $$i -- -Awarnings --emit=llvm-ir,llvm-bc ; \
	done
	@mkdir -p ${TMP_DIR}
	@make -C ${TOP} get-packages-name
	@cat ${TMP_DIR}/.packages-name.tmp | xargs -I {} find ${NEAR_TG_DIR} -name '{}*.bc' > ${TMP_DIR}/.bitcodes.tmp


get-packages-name:
	@python3 ./utils/getPackagesName.py

analysis: unsafe-math round reentrancy div-before-mul transfer timestamp promise-result upgrade-func self-transfer prepaid-gas unhandled-promise yocto-attach complex-loop \
	tautology unused-ret inconsistency lock-callback non-callback-private non-private-callback incorrect-json-type

callback: tg_ir
	@rm -f ${TMP_DIR}/.callback.tmp
	@make -C detectors callback.so
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/callback.so -callback {} -o /dev/null

ext-call-trait: tg_ir
	@rm -f ${TMP_DIR}/.ext-call.tmp
	@make -C detectors ext_call_trait.so
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/ext_call_trait.so -ext-call-trait {} -o /dev/null

ext-call: tg_ir ext-call-trait
	@make -C detectors ext_call.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/ext_call.so -ext-call {} -o /dev/null

complex-loop: tg_ir
	rm -f ${TMP_DIR}/.$@.tmp
	make -C detectors complex_loop.so
	if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/complex_loop.so -complex-loop {} -o /dev/null

unsafe-math: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp ${TMP_DIR}/.$@-toml.tmp
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/unsafe-math-toml.py ${NEAR_SRC_DIR}
	@make -C detectors unsafe_math.so
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/unsafe_math.so -unsafe-math {} -o /dev/null

round: tg_ir
	@make -C detectors round.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/round.so -round {} -o /dev/null

struct-member: tg_ir find-struct
	@make -C detectors struct_member.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/struct_member.so -struct-member {} -o /dev/null

reentrancy: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors reentrancy.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/reentrancy.so -reentrancy {} -o /dev/null

div-before-mul: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors div_before_mul.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/div_before_mul.so -div-before-mul {} -o /dev/null

transfer: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors transfer.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/transfer.so -transfer {} -o /dev/null

timestamp: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors timestamp.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/timestamp.so -timestamp {} -o /dev/null

promise-result: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors promise_result.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/promise_result.so -promise-result {} -o /dev/null

upgrade-func: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors upgrade_func.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/upgrade_func.so -upgrade-func {} -o /dev/null

self-transfer: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors self_transfer.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/self_transfer.so -self-transfer {} -o /dev/null

prepaid-gas: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors prepaid_gas.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/prepaid_gas.so -prepaid-gas {} -o /dev/null

all-call: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors all_call.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/all_call.so -all-call {} -o /dev/null

unhandled-promise: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors unhandled_promise.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/unhandled_promise.so -unhandled-promise {} -o /dev/null

yocto-attach: tg_ir callback
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors yocto_attach.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/yocto_attach.so -yocto-attach {} -o /dev/null

storage-gas: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors storage_gas.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/storage_gas.so -storage-gas {} -o /dev/null

unregistered-receiver: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors unregistered_receiver.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/unregistered_receiver.so -unregistered-receiver {} -o /dev/null

unsaved-changes: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors unsaved_changes.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/unsaved_changes.so -unsaved-changes {} -o /dev/null

nep%-interface: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors nep_interface.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@echo "\e[33m[*] Checking interfaces of NEP-$*\e[0m"  #]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/nep_interface.so -nep-interface --nep-id $* {} -o /dev/null

unclaimed-storage-fee: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors unclaimed_storage_fee.so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/unclaimed_storage_fee.so -unclaimed-storage-fee {} -o /dev/null

nft-approval-check: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors $(subst -,_,$@).so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/$(subst -,_,$@).so -$@ {} -o /dev/null

nft-owner-check: tg_ir
	@rm -f ${TMP_DIR}/.$@.tmp
	@make -C detectors $(subst -,_,$@).so
	@if test $(shell cat ${TMP_DIR}/.bitcodes.tmp | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@cat ${TMP_DIR}/.bitcodes.tmp | xargs -I {} $(LLVM_OPT) ${OPTFLAGS} -load detectors/$(subst -,_,$@).so -$@ {} -o /dev/null

tautology:
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/tautology.py ${NEAR_SRC_DIR}

unused-ret: all-call
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/unused-ret.py ${NEAR_SRC_DIR}

inconsistency:
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/inconsistency.py ${NEAR_SRC_DIR}

lock-callback: callback
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/lock-callback.py ${NEAR_SRC_DIR}

non-callback-private: callback
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/non-callback-private.py ${NEAR_SRC_DIR}

non-private-callback: callback
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/non-private-callback.py ${NEAR_SRC_DIR}

incorrect-json-type: find-struct
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/incorrect-json-type.py ${NEAR_SRC_DIR}

public-interface:
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/public-interface.py ${NEAR_SRC_DIR}

dup-collection-id:
	@rm -f ${TMP_DIR}/.$@.tmp
	@if test $(shell find ${NEAR_SRC_DIR}// -name '*.rs' | wc -c) -gt 0 ; then \
		figlet $@ ; \
	else \
		echo -e "\e[31m[!] Source not found\e[0m" ; \
	fi  # ]]
	@python3 ./detectors/dup-collection-id.py ${NEAR_SRC_DIR}

find-struct:  # provide .struct.tmp and .struct-member.tmp
	@python3 ./utils/findStruct.py ${NEAR_SRC_DIR}

audit: promise-result reentrancy transfer timestamp div-before-mul unsafe-math round find-struct upgrade-func self-transfer prepaid-gas unhandled-promise yocto-attach complex-loop \
	tautology unused-ret inconsistency lock-callback non-callback-private non-private-callback incorrect-json-type
	@python3 ./utils/audit.py ${NEAR_SRC_DIR}

audit-report:
	@python3 ./utils/audit.py ${NEAR_SRC_DIR}

clean: clean_pass clean_example clean_tg
clean_pass:
	make -C detectors clean
clean_example:
	find examples -name "Cargo.toml" | xargs -I {} cargo clean --manifest-path={}
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

compile_flags.txt: Makefile
	echo ${LLVM_CLANG} ${CXXFLAGS} ${LDFLAGS} | sed 's/ /\n/g' > compile_flags.txt

lint:
	rm -f clang-tidy-fixes.yaml
	${LLVM_DIR}/bin/clang-tidy --quiet --export-fixes=clang-tidy-fixes.yaml detectors/*.cpp -- ${CXXFLAGS}

lint-fix:
	${LLVM_DIR}/bin/clang-apply-replacements --style=file ${TOP}

format:
	${LLVM_DIR}/bin/clang-format -i detectors/*.cpp detectors/*.h
	 find . -name "Cargo.toml" | xargs -I {} cargo fmt --manifest-path={}
