/**
 * @file unclaimed_storage_fee.cpp
 * @brief check if there is a check for storage fee balance in `storage_unregister`, the balance should be 0 before withdrawing
 *
 */
#include "near_core.h"

#include <fstream>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct UnclaimedStorageFee : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        llvm::Regex const regex_storage_unregister = llvm::Regex("[0-9]+storage_unregister[0-9]+");

        /**
         * @brief find if there is `icmp eq/ne/ge/le balance, 0` in *F
         *
         * @param F pointer to the target function
         */
        bool hasBalanceCmp(llvm::Function *F, int depth = 3) {
            using namespace llvm;

            if (depth <= 0)
                return false;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F->getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F->getName());

            for (auto &BB : *F) {
                for (auto &I : BB) {
                    if (isa<CallBase>(I) && hasBalanceCmp(dyn_cast<CallBase>(&I)->getCalledFunction(), depth - 1)) {
                        return true;
                    }
                    if (auto *icmpInst = dyn_cast<ICmpInst>(&I)) {
                        if (icmpInst->getOperand(0)->getType()->isIntegerTy(128)) {  // balance is a 128-bit integer (unsigned or signed)
                            // balance == 0, balance != 0
                            if (icmpInst->isEquality() && isa<ConstantInt>(icmpInst->getOperand(1)) && dyn_cast<ConstantInt>(icmpInst->getOperand(1))->getZExtValue() == 0) {
                                // Rustle::Logger().Debug(icmpInst);
                                return true;
                            }
                            // 0 == balance, 0 != balance
                            if (icmpInst->isEquality() && isa<ConstantInt>(icmpInst->getOperand(0)) && dyn_cast<ConstantInt>(icmpInst->getOperand(0))->getZExtValue() == 0) {
                                // Rustle::Logger().Debug(icmpInst);
                                return true;
                            }
                            switch (icmpInst->getPredicate()) {
                                // balance <= 0
                                case ICmpInst::Predicate::ICMP_ULE:
                                    if (isa<ConstantInt>(icmpInst->getOperand(1)) && dyn_cast<ConstantInt>(icmpInst->getOperand(1))->getZExtValue() == 0) {
                                        // Rustle::Logger().Debug(icmpInst);
                                        return true;
                                    }
                                    break;
                                // balance > 0
                                case ICmpInst::Predicate::ICMP_UGT:
                                    if (isa<ConstantInt>(icmpInst->getOperand(1)) && dyn_cast<ConstantInt>(icmpInst->getOperand(1))->getZExtValue() == 0) {
                                        // Rustle::Logger().Debug(icmpInst);
                                        return true;
                                    }
                                    break;
                                // 0 >= balance
                                case ICmpInst::Predicate::ICMP_UGE:
                                    if (isa<ConstantInt>(icmpInst->getOperand(0)) && dyn_cast<ConstantInt>(icmpInst->getOperand(0))->getZExtValue() == 0) {
                                        // Rustle::Logger().Debug(icmpInst);
                                        return true;
                                    }
                                    break;
                                // 0 < balance
                                case ICmpInst::Predicate::ICMP_ULT:
                                    if (isa<ConstantInt>(icmpInst->getOperand(0)) && dyn_cast<ConstantInt>(icmpInst->getOperand(0))->getZExtValue() == 0) {
                                        // Rustle::Logger().Debug(icmpInst);
                                        return true;
                                    }
                                    break;
                                default: break;
                            }
                        }
                    }
                }
            }

            return false;
        }

      public:
        UnclaimedStorageFee() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unclaimed-storage-fee.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnclaimedStorageFee() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            for (auto &F : M.functions()) {
                StringRef const funcFileName;

                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                if (!regex_storage_unregister.match(F.getName()))
                    continue;

                *os << F.getName();

                if (hasBalanceCmp(&F)) {
                    Rustle::Logger().Info("Find storage fee balance check for\e[34m storage_unregister");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack storage fee balance check for\e[34m storage_unregister");
                    *os << "@False\n";
                }
            }
            return false;
        }
    };
}  // namespace

char UnclaimedStorageFee::ID = 0;
static llvm::RegisterPass<UnclaimedStorageFee> X("unclaimed-storage-fee", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnclaimedStorageFee()); });
