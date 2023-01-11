/**
 * @file nft_approval_check.cpp
 * @brief find if there is approval check for nft
 *
 */
#include "near_core.h"

#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct NftApprovalCheck : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        llvm::Regex const regex_standard_nft_transfer_or_call =
            llvm::Regex("near_contract_standards.+non_fungible_token.+core_impl.+NonFungibleToken.+[0-9](nft_transfer(_call)?|internal_transfer)[0-9]");
        llvm::Regex const regex_nft_transfer            = llvm::Regex("[0-9]nft_transfer[0-9]");
        llvm::Regex const regex_nft_transfer_call       = llvm::Regex("[0-9]nft_transfer_call[0-9]");
        llvm::Regex const regex_nft_transfer_trait      = llvm::Regex("near_contract_standards\\.\\.non_fungible_token\\.\\.core\\.\\.NonFungibleTokenCore\\$.+[0-9]nft_transfer[0-9]");
        llvm::Regex const regex_nft_transfer_call_trait = llvm::Regex("near_contract_standards\\.\\.non_fungible_token\\.\\.core\\.\\.NonFungibleTokenCore\\$.+[0-9]nft_transfer_call[0-9]");

      public:
        NftApprovalCheck() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.nft-approval-check.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~NftApprovalCheck() { os->close(); }

        bool hasApprovalCheck(llvm::Function *F, unsigned approvalId0Offset) {
            using namespace llvm;

            auto *const approvalId0 = F->getArg(approvalId0Offset);  // approval_id is a `Option<u64>`, in llvm-ir it has approval_id.0 and approval.1

            // Use implementation in `near_contract_standards`
            if (regex_standard_nft_transfer_or_call.match(F->getName()))
                return true;

            std::set<Value *> usersOfApprovalId0;
            Rustle::simpleFindUsers(approvalId0, usersOfApprovalId0, false, true);

            Value *approvalId = nullptr;

            for (auto *i : usersOfApprovalId0) {
                if (auto *stInst = dyn_cast<StoreInst>(i)) {
                    if (auto *gepInst = dyn_cast<GetElementPtrInst>(stInst->getPointerOperand())) {
                        if (Rustle::printToString(gepInst->getSourceElementType()) == "{ i64, i64 }") {
                            approvalId = gepInst->getPointerOperand();
                        }
                    }
                }
            }

            if (approvalId == nullptr) {  // approval_id not found (should happens)
                return false;
            }

            std::set<Value *> usersOfApprovalId;
            Rustle::simpleFindUsers(approvalId, usersOfApprovalId, false, true);

            for (BasicBlock &BB : *F) {
                for (Instruction &I : BB) {
                    if (Rustle::isInstCallFunc(&I, Rustle::regexPartialEq)) {
                        for (auto &argUse : dyn_cast<CallBase>(&I)->args()) {
                            if (usersOfApprovalId.count(argUse.get())) {
                                return true;
                            }
                        }
                    } else if (auto *callInst = dyn_cast<CallBase>(&I)) {  // check next function iteratively
                        int nextApprovalIdOffset = -1;
                        for (int i = 0; i < callInst->arg_size(); i++) {
                            if (usersOfApprovalId0.count(callInst->getArgOperand(i))) {  // whether passing ft_transfer's arg receiver to next level
                                nextApprovalIdOffset = i;
                                break;
                            }
                        }
                        if (nextApprovalIdOffset == -1)  // not found, skip this Instruction
                            continue;

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

            if (regex_nft_transfer.match(F.getName())) {
                if (F.arg_size() < 6)
                    return false;

                Rustle::Logger().Info("Find nft_transfer \e[34m", F.getName());
                *os << F.getName();

                if (hasApprovalCheck(&F, 3)) {
                    Rustle::Logger().Info("Find approval check for\e[34m nft_transfer");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack approval check for\e[34m nft_transfer");
                    *os << "@False\n";
                }
            } else if (regex_nft_transfer_call.match(F.getName())) {
                if (F.arg_size() < 7)  // return value in arg[0] not counted since it may not be in args
                    return false;

                Rustle::Logger().Info("Find nft_transfer_call \e[34m", F.getName());
                *os << F.getName();

                if (hasApprovalCheck(&F, 4)) {
                    Rustle::Logger().Info("Find approval check for\e[34m nft_transfer_call");
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack approval check for\e[34m nft_transfer_call");
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
