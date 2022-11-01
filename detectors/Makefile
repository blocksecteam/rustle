.PHONY: pass clean

DEP = near_core.h #$(shell find ../ -name Makefile)

pass: callback.so ext_call_trait.so ext_call.so complex_loop.so unsafe_math.so round.so struct_member.so reentrancy.so div_before_mul.so transfer.so timestamp.so promise_result.so upgrade_func.so self_transfer.so prepaid_gas.so all_call.so unhandled_promise.so yocto_attach.so

callback.so: callback.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
ext_call_trait.so: ext_call_trait.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
ext_call.so: ext_call.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
complex_loop.so: complex_loop.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
unsafe_math.so: unsafe_math.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
round.so: round.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
struct_member.so: struct_member.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
reentrancy.so: reentrancy.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
div_before_mul.so: div_before_mul.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
transfer.so: transfer.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
timestamp.so: timestamp.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
promise_result.so: promise_result.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
upgrade_func.so: upgrade_func.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
self_transfer.so: self_transfer.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
prepaid_gas.so: prepaid_gas.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
all_call.so: all_call.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
unhandled_promise.so: unhandled_promise.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}
yocto_attach.so: yocto_attach.cpp ${DEP}
	$(LLVM_CLANG) ${CXXFLAGS} $< -o $@ ${LDFLAGS}

clean:
	rm -rf *.so
