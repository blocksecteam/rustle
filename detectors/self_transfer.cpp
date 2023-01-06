/**
 * @file self_transfer.cpp
 * @brief find if ft_transfer checks `sender_id != receiver_id`
 *
 */
#include "near_core.h"

#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct SelfTransfer : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        const llvm::Regex regex_standard_ft_transfer_or_call = llvm::Regex("near_contract_standards.+fungible_token.+core_impl.+FungibleToken.+[0-9](ft_transfer(_call)?|internal_transfer)[0-9]");
        const llvm::Regex regex_ft_transfer_trait            = llvm::Regex("near_contract_standards\\.\\.fungible_token\\.\\.core\\.\\.FungibleTokenCore\\$.+[0-9]ft_transfer[0-9]");
        const llvm::Regex regex_ft_transfer_call_trait       = llvm::Regex("near_contract_standards\\.\\.fungible_token\\.\\.core\\.\\.FungibleTokenCore\\$.+[0-9]ft_transfer_call[0-9]");

      public:
        SelfTransfer() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.self-transfer.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~SelfTransfer() { os->close(); }

        bool hasSenderReceiverCheck(llvm::Function *F, unsigned receiverOffset) {
            using namespace llvm;

            // Use implementation in `near_contract_standards`
            if (regex_standard_ft_transfer_or_call.match(F->getName()))
                return true;

            std::set<Value *> usersOfReceiverId;
            Rustle::findUsers(F->getArg(receiverOffset), usersOfReceiverId);

            for (BasicBlock &BB : *F) {
                for (Instruction &I : BB) {
                    if (Rustle::isInstCallFunc(&I, Rustle::regexPartialEq)) {
                        bool useReceiverId = false;
                        for (int i = 0; i < dyn_cast<CallBase>(&I)->arg_size(); i++) {
                            std::string const typeName = Rustle::printToString(dyn_cast<CallBase>(&I)->getArgOperand(i)->getType());
                            if (StringRef(typeName).contains("%\"near_sdk::types::account_id::AccountId\"") || StringRef(typeName).contains("%\"alloc::string::String\"")) {
                                useReceiverId = true;
                                break;
                            }
                        }

                        if (useReceiverId)
                            return true;
                    } else if (auto *callInst = dyn_cast<CallBase>(&I)) {
                        int nextReceiverOffset = -1;
                        for (int i = 0; i < callInst->arg_size(); i++) {
                            if (usersOfReceiverId.count(callInst->getArgOperand(i))) {  // whether passing ft_transfer's arg receiver to next level
                                nextReceiverOffset = i;
                                break;
                            }
                        }
                        if (nextReceiverOffset == -1)  // not found, skip this Instruction
                            continue;

                        if (callInst->getCalledFunction() && hasSenderReceiverCheck(callInst->getCalledFunction(), nextReceiverOffset)) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;
            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            if (regex_ft_transfer_trait.match(F.getName())) {
                if (F.arg_size() < 3)
                    return false;
                // std::string typeName = Rustle::printToString(F.getArg(1)->getType());
                // if (!StringRef(typeName).contains("AccountId"))
                //     return false;

                Rustle::Logger().Info("Find ft_transfer \e[34m", F.getName());
                *os << F.getName();

                if (hasSenderReceiverCheck(&F, 1)) {
                    Rustle::Logger().Info("Find self-transfer check for \e[34mft_transfer");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack self-transfer check for \e[34mft_transfer");
                    *os << "@False\n";
                }
            } else if (regex_ft_transfer_call_trait.match(F.getName())) {
                if (F.arg_size() < 3)
                    return false;
                // std::string typeName = Rustle::printToString(F.getArg(2)->getType());
                // if (!StringRef(typeName).contains("AccountId"))
                //     return false;

                Rustle::Logger().Info("Find ft_transfer_call \e[34m", F.getName());
                *os << F.getName();

                if (hasSenderReceiverCheck(&F, 2)) {
                    Rustle::Logger().Info("Find self-transfer check for \e[34mft_transfer_call");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack self-transfer check for \e[34mft_transfer_call");
                    *os << "@False\n";
                }
            }

            return false;
        }
    };
}  // namespace

char SelfTransfer::ID = 0;
static llvm::RegisterPass<SelfTransfer> X("self-transfer", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new SelfTransfer()); });
