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

        const llvm::Regex regex_standard_nft_transfer_or_call = llvm::Regex("near_contract_standards..non_fungible_token..core_impl..NonFungibleToken\\$.+[0-9]nft_transfer(_call)?[0-9]");
        const llvm::Regex regex_nft_transfer_or_call          = llvm::Regex("[0-9]nft_transfer(_call)?[0-9]");
        const llvm::Regex regex_nft_transfer_trait            = llvm::Regex("near_contract_standards..non_fungible_token..core..NonFungibleTokenCore\\$.+[0-9]nft_transfer[0-9]");
        const llvm::Regex regex_nft_transfer_call_trait       = llvm::Regex("near_contract_standards..non_fungible_token..core..NonFungibleTokenCore\\$.+[0-9]nft_transfer_call[0-9]");

      public:
        NftApprovalCheck() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.self-transfer.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~NftApprovalCheck() { os->close(); }

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
            }

            return false;
        }
    };
}  // namespace

char NftApprovalCheck::ID = 0;
static llvm::RegisterPass<NftApprovalCheck> X("nft-approval-check", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new NftApprovalCheck()); });
