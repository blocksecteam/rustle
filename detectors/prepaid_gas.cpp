/**
 * @file prepaid_gas.cpp
 * @brief check if there is a check for prepaid_gas in ft_transfer_call
 *
 */
#include "near_core.h"

#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct PrepaidGas : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        PrepaidGas() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.prepaid-gas.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~PrepaidGas() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;
            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            if (!Regex("near_contract_standards..fungible_token..core..FungibleTokenCore\\$.+[0-9]ft_transfer_call[0-9]").match(F.getName()))
                return false;

            Rustle::Logger().Info("Find ft_transfer ", F.getName());
            *os << F.getName();

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    auto rePrepaidGas = Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+prepaid_gas");
                    if (Rustle::isInstCallFunc(&I, rePrepaidGas)) {
                        bool checkPrepaidGas = false;

                        std::set<Value *> prepaidGasUsers;
                        Rustle::findUsers(&I, prepaidGasUsers);
                        for (auto *i : prepaidGasUsers) {
                            if (auto *callBase = dyn_cast<CallBase>(i)) {
                                if (Rustle::isInstCallFunc(callBase, Rustle::regexPartialOrd)) {
                                    checkPrepaidGas = true;
                                    break;
                                }
                            }
                        }
                        if (checkPrepaidGas) {
                            Rustle::Logger().Info("Find prepaid_gas check in ft_transfer_call at ", &I.getDebugLoc());
                            *os << "@True\n";
                            return false;
                        }
                    }
                }
            }

            Rustle::Logger().Warning("Lack prepaid_gas check in ft_transfer_call");
            *os << "@False\n";

            return false;
        }
    };
}  // namespace

char PrepaidGas::ID = 0;
static llvm::RegisterPass<PrepaidGas> X("prepaid-gas", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new PrepaidGas()); });
