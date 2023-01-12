/**
 * @file nft_owner_check.cpp
 * @brief check if there is an owner check in priviledged function
 *
 */
#include "near_core.h"

#include <fstream>
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
    struct NftOwnerCheck : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        std::set<std::string> callbacks;

        llvm::Regex const regex_nft_approve_function = llvm::Regex("[0-9]+(nft_approve|nft_revoke|nft_revoke_all)[0-9]+");  // `nft_revoke_all` is not priviledged

      public:
        NftOwnerCheck() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.nft-owner-check.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);

            std::ifstream is;
            is.open(Rustle::callback_file);
            std::string callbackLine;
            while (is >> callbackLine) {
                callbacks.insert(callbackLine.substr(0, callbackLine.find('@')));
            }
            is.close();
        }
        ~NftOwnerCheck() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            for (auto &F : M.functions()) {
                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (F.getName().contains("$closure$") || !regex_nft_approve_function.match(F.getName()))  // only check selected functions
                    continue;

                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                *os << F.getName();
                if (Rustle::isFuncPrivileged(&F)) {
                    Rustle::Logger().Info("Find owner check for \e[34m ", F.getName());
                    *os << "@True\n";
                } else {
                    Rustle::Logger().Warning("Lack owner check for \e[34m", F.getName());
                    *os << "@False\n";
                }
            }
            return false;
        }
    };
}  // namespace

char NftOwnerCheck::ID = 0;
static llvm::RegisterPass<NftOwnerCheck> X("nft-owner-check", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new NftOwnerCheck()); });
