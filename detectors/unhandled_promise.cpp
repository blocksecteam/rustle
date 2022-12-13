/**
 * @file unhandled_promise.cpp
 * @brief find all promises not handled by `then` or `and`
 *
 */
#include "near_core.h"

#include <set>

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
    struct UnhandledPromise : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        UnhandledPromise() : FunctionPass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unhandled-promise.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);

            if (Rustle::debug_print_tmp)
                llvm::outs() << "[*] Reading ext call list\n";
        }
        ~UnhandledPromise() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            for (llvm::BasicBlock &BB : F)
                for (llvm::Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(&I)) {
                        if (!callInst->getCalledFunction())
                            continue;
                        if (callInst->arg_size() > 1 && callInst->getArgOperand(0)->getType() &&
                            Rustle::printToString(callInst->getArgOperand(0)).find("near_sdk::promise::Promise") != std::string::npos) {  // return value is promise
                            bool handlePromise = false;
                            bool usedInReturn  = false;

                            std::set<Value *> pmUsers;
                            Rustle::simpleFindUsers(callInst->getArgOperand(0), pmUsers, true);  // find users of promises
                            for (auto *i : pmUsers) {
                                if (auto *retInst = dyn_cast<ReturnInst>(i)) {  // find if store location is used in return value of `F`
                                    usedInReturn = true;
                                    break;
                                }
                                if (F.arg_size() >= 1 && F.getArg(0) == i) {
                                    usedInReturn = true;
                                    break;
                                }
                            }

                            if (F.arg_size() >= 1 && F.getArg(0) == callInst->getArgOperand(0)) {  // the return value of callInst will be returned by F directly
                                usedInReturn = true;
                            } else {  // find if it's handled by other function
                                for (auto *user : callInst->getArgOperand(0)->users()) {
                                    if (llvm::CallBase *callThen = llvm::dyn_cast<llvm::CallBase>(user)) {
                                        if (Regex("(.+near_sdk[0-9]+promise[0-9]+Promise[0-9]+([_a-z]+)[0-9]+[0-9a-z]+)").match(callThen->getCalledFunction()->getName())) {
                                            handlePromise = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!handlePromise && !usedInReturn) {
                                Rustle::Logger().Warning("Unhandled promise of ", callInst->getCalledFunction()->getName(), " at ", I.getDebugLoc());
                                *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                            }
                        }
                    }
                }

            return false;
        }
    };
}  // namespace

char UnhandledPromise::ID = 0;
static llvm::RegisterPass<UnhandledPromise> X("unhandled-promise", "functions calling external function", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnhandledPromise()); });
