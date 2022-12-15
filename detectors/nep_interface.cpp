/**
 * @file nep_interface.cpp
 * @brief check if there is unimplemented NEP interface
 * @ref NEP "https://github.com/near/NEPs"
 *
 */
#include "near_core.h"

#include <set>
#include <unordered_map>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static llvm::cl::opt<unsigned> NepIdArg("nep-id", llvm::cl::desc("Specify the id of nep, refer to https://github.com/near/NEPs for more"), llvm::cl::value_desc("id"));

/**
 * @example {
 *     {
 *         nep-id,  // NEP Title
 *         {func1, func2, func3, ...},
 *     },
 * }
 *
 */
static const std::unordered_map<unsigned, std::set<llvm::StringRef>> positionalFuncsMap = {
    {
        141,  // Fungible Token Standard
        {"ft_transfer", "ft_transfer_call", "ft_total_supply", "ft_balance_of"},
    },
    {
        145,  // Storage Management
        {"storage_deposit", "storage_withdraw", "storage_unregister", "storage_balance_bounds", "storage_balance_of"},
    },
    {
        148,  // Fungible Token Metadata
        {"ft_metadata"},
    },
    {
        171,  // Non Fungible Token Standard
        {"nft_transfer", "nft_transfer_call", "nft_token"},
    },
    {
        177,  // Non Fungible Token Metadata
        {"nft_metadata"},
    },
    {
        178,  // Non Fungible Token Approval Management
        {"nft_approve", "nft_revoke", "nft_revoke_all", "nft_is_approved"},
    },
    {
        181,  // Non Fungible Token Enumeration
        {"nft_total_supply", "nft_tokens", "nft_supply_for_owner", "nft_tokens_for_owner"},
    },
    {
        199,  // Non Fungible Token Royalties and Payouts
        {},
    },
    {
        245,  // Multi Token Standard
        {"mt_transfer", "mt_batch_transfer", "mt_transfer_call", "mt_batch_transfer_call", "mt_token", "mt_balance_of", "mt_batch_balance_of", "mt_supply", "mt_batch_supply"},
    },
    {
        297,  // Events Standard
        {},
    },
    {
        330,  // Source Metadata
        {},
    },
    {
        366,  // Meta Transactions
        {},
    },
};

/**
 * @brief functions without specified names, the names are up to developers, here only provide some possible name options
 * @example {
 *     {
 *         nep-id,
 *         {
 *             {func1_name1, func1_name2},
 *             {func2_name1, ...},
 *         },
 *     },
 * }
 *
 */
static const std::unordered_map<unsigned, std::set<std::set<llvm::StringRef>>> optionalFuncMap = {
    {
        141,
        {
            {"ft_resolve_transfer", "resolve_transfer"},
        },
    },
    {
        171,
        {
            {"nft_resolve_transfer", "resolve_transfer"},
        },
    },
    {
        245,
        {
            {"mft_resolve_transfer", "resolve_transfer"},
        },
    },
};

namespace {
    struct UnsavedChanges : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        UnsavedChanges() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.nep-interface.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnsavedChanges() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            auto const &nepId = NepIdArg.getValue();

            if (positionalFuncsMap.count(nepId) == 0)  // check the existence of nepId
                Rustle::Logger().Error("Invalid nep-id: ", nepId);

            std::set<StringRef> funcSet;

            for (auto &F : M.functions()) {
                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                funcSet.insert(F.getName());
            }

            auto const &positionalFuncs = positionalFuncsMap.at(nepId);  // existence has been checked
            auto const &optionalFuncs   = optionalFuncMap.count(nepId) ? optionalFuncMap.at(nepId) : std::set<std::set<llvm::StringRef>>();

            // check positional functions' implementation
            for (auto const &func : positionalFuncs) {
                if (funcSet.count(func)) {
                    Rustle::Logger().Info("Implemented:   ", func);
                } else {
                    Rustle::Logger().Warning("Unimplemented: ", func);
                }
            }

            // check optional functions' implementation
            for (auto const &funcGroup : optionalFuncs) {
                for (auto const &func : funcGroup) {
                    if (funcSet.count(func)) {
                        Rustle::Logger().Info("Implemented:   ", func);
                        break;  // skip current group
                    }
                }
            }

            // check resolver of transfer call
            switch (nepId) {
                case 141: break;
                case 171: break;
                default: break;
            }

            return false;
        }
    };
}  // namespace

char UnsavedChanges::ID = 0;

static llvm::RegisterPass<UnsavedChanges> X("nep-interface", "", false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnsavedChanges()); });
