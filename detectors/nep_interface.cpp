/**
 * @file nep_interface.cpp
 * @brief check if there is unimplemented NEP interface
 * @ref NEP "https://github.com/near/NEPs"
 *
 */
#include "near_core.h"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

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
            {"mt_resolve_transfer", "resolve_transfer"},
        },
    },
};

static llvm::cl::opt<unsigned> NepIdArg("nep-id", llvm::cl::desc("Specify the id of nep, refer to https://github.com/near/NEPs for more"), llvm::cl::value_desc("id"));

namespace {
    struct UnsavedChanges : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os     = nullptr;
        llvm::Regex const regex_then = llvm::Regex("near_sdk[0-9]+promise[0-9]+Promise[0-9]+then");

      public:
        UnsavedChanges() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.nep" + std::to_string(NepIdArg.getValue()) + "-interface.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
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
            for (StringRef const &func : positionalFuncs) {
                if (std::find_if(funcSet.begin(), funcSet.end(), [func](StringRef const &funcName) { return funcName == func || Regex("[0-9]+" + func.str() + "[0-9]+").match(funcName); }) !=
                    funcSet.end()) {
                    Rustle::Logger().Info("Implemented: ", func);
                } else {
                    Rustle::Logger().Warning("Unimplemented: ", func);
                    *os << func << '\n';
                }
            }

            // // check optional functions' implementation
            // for (auto const &funcGroup : optionalFuncs) {
            //     for (auto const &func : funcGroup) {
            //         if (std::find_if(funcSet.begin(), funcSet.end(),
            //                 [func](StringRef const &funcName)
            //                 { return funcName == func || Regex("[0-9]+" + func.str() + "[0-9]+").match(funcName); }) != funcSet.end()) {
            //             Rustle::Logger().Info("Implemented:   ", func);
            //             break;  // skip current group
            //         }
            //     }
            // }

            // check resolver of transfer call
            std::set<unsigned> static const nepWithResolver = {141, 171, 245};
            if (nepWithResolver.count(nepId)) {
                bool foundResolver = false;
                for (auto &F : M) {
                    // only check function xx_transfer_call
                    switch (nepId) {
                        case 141:
                            if (F.getName() != "ft_transfer_call" && !Regex("[0-9]+ft_transfer_call[0-9]+").match(F.getName()))
                                continue;
                            break;
                        case 171:
                            if (F.getName() != "nft_transfer_call" && !Regex("[0-9]+nft_transfer_call[0-9]+").match(F.getName()))
                                continue;
                            break;
                        case 245:
                            if (F.getName() != "mt_transfer_call" && !Regex("[0-9]+mt_transfer_call[0-9]+").match(F.getName()))
                                continue;
                            break;
                    }

                    for (auto &BB : F) {
                        for (auto &I : BB) {
                            llvm::Regex regexOnTransfer;
                            switch (nepId) {
                                case 141: regexOnTransfer = Regex("[0-9]+ft_on_transfer[0-9]+"); break;
                                case 171: regexOnTransfer = Regex("[0-9]+nft_on_transfer[0-9]+"); break;
                                case 245: regexOnTransfer = Regex("[0-9]+mt_on_transfer[0-9]+"); break;
                            }

                            if (Rustle::isInstCallFunc(&I, regexOnTransfer)) {
                                std::set<Value *> usersPromiseOfOnTransfer;
                                if (dyn_cast<CallBase>(&I)->arg_size() >= 1) {
                                    Rustle::simpleFindUsers(dyn_cast<CallBase>(&I)->getArgOperand(0), usersPromiseOfOnTransfer);
                                    for (auto *userOfOnTransfer : usersPromiseOfOnTransfer) {
                                        if (isa<CallBase>(userOfOnTransfer) && Rustle::isInstCallFunc(dyn_cast<Instruction>(userOfOnTransfer), regex_then)) {
                                            auto const *nextPromiseResult = dyn_cast<CallBase>(userOfOnTransfer)->getArgOperand(dyn_cast<CallBase>(userOfOnTransfer)->arg_size() - 1);  // last arg
                                            for (auto *userOfResolveTransfer : nextPromiseResult->users()) {
                                                if (isa<CallBase>(userOfResolveTransfer) && userOfResolveTransfer != userOfOnTransfer) {
                                                    // Rustle::Logger().Info("Implemented(resolver): ",dyn_cast<CallBase>(userOfResolveTransfer)->getCalledFunction()->getName());
                                                    switch (nepId) {
                                                        case 141: Rustle::Logger().Info("Implemented: resolver of ft_transfer_call"); break;
                                                        case 171: Rustle::Logger().Info("Implemented: resolver of nft_transfer_call"); break;
                                                        case 245: Rustle::Logger().Info("Implemented: resolver of mt_transfer_call"); break;
                                                    }
                                                    foundResolver = true;
                                                    goto EXIT_FINDING_RESOLVER;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                EXIT_FINDING_RESOLVER:
                    if (!foundResolver) {
                        switch (nepId) {
                            case 141:
                                Rustle::Logger().Warning("Unimplemented: resolver of ft_transfer_call");
                                *os << "resolver of ft_transfer_call\n";
                                break;
                            case 171:
                                Rustle::Logger().Warning("Unimplemented: resolver of nft_transfer_call");
                                *os << "resolver of nft_transfer_call\n";
                                break;
                            case 245:
                                Rustle::Logger().Warning("Unimplemented: resolver of mt_transfer_call");
                                *os << "resolver of mt_transfer_call\n";
                                break;
                        }
                    }
                }
            }

            return false;
        }
    };
}  // namespace

char UnsavedChanges::ID = 0;

static llvm::RegisterPass<UnsavedChanges> X("nep-interface", "", false /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnsavedChanges()); });
