/**
 * @file nft_approval_check.cpp
 * @brief find if there is approval check for nft
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
    struct NftApprovalCheck : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        const llvm::Regex regex_standard_nft_transfer_or_call =
            llvm::Regex("near_contract_standards.+non_fungible_token.+core_impl.+NonFungibleToken.+[0-9](nft_transfer(_call)?|internal_transfer)[0-9]");
        const llvm::Regex regex_nft_transfer_or_call    = llvm::Regex("[0-9]nft_transfer(_call)?[0-9]");
        const llvm::Regex regex_nft_transfer_trait      = llvm::Regex("near_contract_standards\\.\\.non_fungible_token\\.\\.core\\.\\.NonFungibleTokenCore\\$.+[0-9]nft_transfer[0-9]");
        const llvm::Regex regex_nft_transfer_call_trait = llvm::Regex("near_contract_standards\\.\\.non_fungible_token\\.\\.core\\.\\.NonFungibleTokenCore\\$.+[0-9]nft_transfer_call[0-9]");

      public:
        NftApprovalCheck() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.nft-approval-check.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~NftApprovalCheck() { os->close(); }

        bool hasApprovalCheck(llvm::Function *F, unsigned approvalIdOffset) {
            using namespace llvm;

            Rustle::Logger().Debug(F->getName());
            Rustle::Logger().Debug(F->getArg(approvalIdOffset));

            // Use implementation in `near_contract_standards`
            if (regex_standard_nft_transfer_or_call.match(F->getName()))
                return true;

            std::set<Value *> usersOfApprovalId;
            Rustle::simpleFindUsers(F->getArg(approvalIdOffset), usersOfApprovalId, false, true);

            for (BasicBlock &BB : *F) {
                for (Instruction &I : BB) {
                    if (Rustle::isInstCallFunc(&I, Rustle::regexPartialEq)) {
                        // bool useReceiverId = false;
                        // for (int i = 0; i < dyn_cast<CallBase>(&I)->arg_size(); i++) {
                        //     std::string const typeName = Rustle::printToString(dyn_cast<CallBase>(&I)->getArgOperand(i)->getType());
                        //     if (StringRef(typeName).contains("%\"near_sdk::types::account_id::AccountId\"") || StringRef(typeName).contains("%\"alloc::string::String\"")) {
                        //         useReceiverId = true;
                        //         break;
                        //     }
                        // }

                        // if (useReceiverId)
                        //     return true;
                    } else if (auto *callInst = dyn_cast<CallBase>(&I)) {
                        int nextApprovalIdOffset = -1;
                        for (int i = 0; i < callInst->arg_size(); i++) {
                            if (usersOfApprovalId.count(callInst->getArgOperand(i))) {  // whether passing ft_transfer's arg receiver to next level
                                nextApprovalIdOffset = i;
                                break;
                            }
                        }
                        if (nextApprovalIdOffset == -1)  // not found, skip this Instruction
                            continue;

                        Rustle::Logger().Debug(callInst);
                        Rustle::Logger().Debug(nextApprovalIdOffset);
                        Rustle::Logger().Debug(callInst->getArgOperand(nextApprovalIdOffset));

                        if (callInst->getCalledFunction() && hasApprovalCheck(callInst->getCalledFunction(), nextApprovalIdOffset)) {
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

            if (regex_nft_transfer_or_call.match(F.getName())) {
                if (F.arg_size() < 3)
                    return false;

                Rustle::Logger().Info("Find ft_transfer \e[34m", F.getName());
                *os << F.getName();

                if (hasApprovalCheck(&F, 3)) {
                    Rustle::Logger().Info("Find approval check for\e[34m nft transfer");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack approval check for\e[34m nft transfer");
                    *os << "@False\n";
                }
            }

            return false;
        }
    };
}  // namespace

char NftApprovalCheck::ID = 0;
static llvm::RegisterPass<NftApprovalCheck> X("nft-approval-check", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new NftApprovalCheck()); });
