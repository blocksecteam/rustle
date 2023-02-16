/**
 * @file reentrancy.cpp
 * @brief find possible reentrancy
 *
 */
#include "near_core.h"

#include <fstream>
#include <set>
#include <string>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct Reentrancy : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        Reentrancy() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.reentrancy.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~Reentrancy() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;

            StringRef const funcFileName;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    if (CallBase *callInst = dyn_cast<CallBase>(&I)) {
                        if (!callInst->getCalledFunction())
                            continue;
                        if (Rustle::regexPromiseResult.match(callInst->getCalledFunction()->getName())) {
                            BasicBlock *successfulBlock = nullptr;

                            std::set<Value *> pmRsUsers;
                            Rustle::simpleFindUsers(callInst->getArgOperand(0), pmRsUsers, false, true);  // callInst->getArgOperand(0) is the return value of `promise_result`
                            for (auto *i : pmRsUsers) {
                                if (auto *switchInst = dyn_cast<SwitchInst>(i)) {
                                    for (auto caseIt : switchInst->cases()) {
                                        if (caseIt.getCaseValue()->equalsInt(1)) {  // PromiseResult::Successful == 1
                                            successfulBlock = caseIt.getCaseSuccessor();
                                            break;
                                        }
                                    }
                                }
                                if (successfulBlock)
                                    break;
                            }

                            if (successfulBlock) {
                                bool hasNewBlock = true;

                                auto *currentBlock = successfulBlock;
                                while (hasNewBlock) {
                                    hasNewBlock = false;

                                    // Rustle::Logger().Debug(currentBlock->getName());

                                    for (Instruction &I : *currentBlock) {
                                        // branch inst, branch between basic block
                                        if (auto *branchInst = dyn_cast<BranchInst>(&I)) {
                                            for (unsigned i = 0; i < branchInst->getNumSuccessors(); i++) {
                                                if (branchInst->getSuccessor(i)->getName().startswith("bb")) {  // ignore panic or so
                                                    currentBlock = branchInst->getSuccessor(i);
                                                    hasNewBlock  = true;
                                                }
                                            }
                                        }

                                        // store inst, check store
                                        if (auto *storeInst = dyn_cast<StoreInst>(&I)) {
                                            bool usedInReturn = false;
                                            bool useSelf      = false;

                                            std::set<Value *> stLocUsers;
                                            Rustle::simpleFindUsers(storeInst->getPointerOperand(), stLocUsers);  // find users of store location
                                            for (auto *i : stLocUsers) {
                                                if (auto *retInst = dyn_cast<ReturnInst>(i)) {  // find if store location is used in return value of `F`

                                                    // [!] below is code to strictly check whether return type equals stored value's type, but it's unnecessary in most cases
                                                    // if (retInst->getReturnValue() && storeInst->getValueOperand()->getType() == retInst->getReturnValue()->getType()) {
                                                    usedInReturn = true;
                                                    break;
                                                    // }
                                                }
                                            }

                                            std::set<Value *> selfUsers;
                                            Rustle::simpleFindUsers(F.getArg(0), selfUsers);  // find users of `&self`
                                            for (auto *i : selfUsers) {
                                                if (i == storeInst->getPointerOperand()) {
                                                    useSelf = true;
                                                    break;
                                                }
                                            }

                                            if (!usedInReturn && useSelf) {
                                                Rustle::Logger().Warning("Changing state upon PromiseResult::Successful at ", I.getDebugLoc(), " may lead to reentrancy.");
                                                *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                                                return false;
                                            }
                                        }
                                    }
                                }

                                return false;
                            }
                        }
                    }
                }

            return false;
        }
    };

}  // namespace

char Reentrancy::ID = 0;
static llvm::RegisterPass<Reentrancy> X("reentrancy", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new Reentrancy()); });
