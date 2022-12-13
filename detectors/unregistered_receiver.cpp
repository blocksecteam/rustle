/**
 * @file unregistered_receiver.cpp
 * @brief find if program will panic on unregistered transfer receivers
 *
 */
#include "near_core.h"

#include <set>

#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct UnregisteredReceiver : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        const llvm::Regex regex_ft_transfer_or_call    = llvm::Regex("near_contract_standards..fungible_token..core_impl..FungibleToken\\$.+[0-9]ft_transfer(_call)?[0-9]");
        const llvm::Regex regex_ft_transfer_trait      = llvm::Regex("near_contract_standards..fungible_token..core..FungibleTokenCore\\$.+[0-9]ft_transfer[0-9]");
        const llvm::Regex regex_ft_transfer_call_trait = llvm::Regex("near_contract_standards..fungible_token..core..FungibleTokenCore\\$.+[0-9]ft_transfer_call[0-9]");

        const llvm::Regex regex_get = llvm::Regex("near_sdk[0-9]+collections[0-9]+"  // `get` in map collections
                                                  "(lookup_map[0-9]+LookupMap|tree_map[0-9]+TreeMap|unordered_map[0-9]+UnorderedMap|legacy_tree_map[0-9]+LegacyTreeMap)"
                                                  "\\$.+[0-9]+get[0-9]+");
        const llvm::Regex regex_unchecked_unwrap =
            llvm::Regex("core[0-9]+(option[0-9]+Option|result[0-9]+Result)\\$.+[0-9]+(unwrap_or|unwrap_or_default|unwrap_unchecked)[0-9]+");  // unwrap and unwrap_or_else is ok

      public:
        UnregisteredReceiver() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unregistered-receiver.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnregisteredReceiver() { os->close(); }

        bool allowUnregisteredReceiver(llvm::Function *F, llvm::Value *receiver) {
            using namespace llvm;

            // Use implementation in `near_contract_standards`
            if (regex_ft_transfer_or_call.match(F->getName()))
                return true;

            std::set<Value *> usersOfReceiverId;
            Rustle::simpleFindUsers(receiver, usersOfReceiverId, false, true);

            for (auto *v : usersOfReceiverId) {
                if (auto *callInst = dyn_cast<CallBase>(v)) {
                    if (Rustle::isInstCallFunc(callInst, regex_get)) {
                        Value *returnValueOfGet = nullptr;
                        switch (dyn_cast<CallBase>(callInst)->arg_size()) {
                            case 2: returnValueOfGet = callInst; break;                                        // returnVal = call get(self, key)
                            case 3: returnValueOfGet = dyn_cast<CallBase>(callInst)->getArgOperand(0); break;  // call get(returnVal, self, key)
                            default: break;
                        }
                        if (returnValueOfGet == nullptr) {  // skip other circumstances
                            continue;
                        }

                        std::set<Value *> usersOfGet;
                        Rustle::simpleFindUsers(returnValueOfGet, usersOfGet);

                        for (auto *v : usersOfGet) {  // find if Option of `get` is unwrapped without check
                            if (auto *callInst = dyn_cast<CallBase>(v)) {
                                if (Rustle::isInstCallFunc(callInst, regex_unchecked_unwrap)) {
                                    return true;
                                }
                            }
                        }
                    } else {
                        int nextReceiverOffset = -1;
                        for (int i = 0; i < callInst->arg_size(); i++) {
                            if (usersOfReceiverId.count(callInst->getArgOperand(i))) {  // whether passing ft_transfer's arg receiver to next level
                                nextReceiverOffset = i;
                                break;
                            }
                        }
                        if (nextReceiverOffset == -1)  // not found, skip this Instruction
                            continue;

                        if (callInst->getCalledFunction() && allowUnregisteredReceiver(callInst->getCalledFunction(), callInst->getCalledFunction()->getArg(nextReceiverOffset))) {
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

                Rustle::Logger().Info("Find ft_transfer \e[34m", F.getName());
                *os << F.getName();

                if (allowUnregisteredReceiver(&F, F.getArg(1))) {
                    Rustle::Logger().Warning("Unregistered receiver can be used for \e[34mft_transfer");
                    *os << "@False\n";
                } else {
                    Rustle::Logger().Info("Unregistered receiver can't be used for \e[34mft_transfer");
                    *os << "@True\n";
                }
            } else if (regex_ft_transfer_call_trait.match(F.getName())) {
                if (F.arg_size() < 3)
                    return false;

                Rustle::Logger().Info("Find ft_transfer_call \e[34m", F.getName());
                *os << F.getName();

                if (allowUnregisteredReceiver(&F, F.getArg(2))) {
                    Rustle::Logger().Warning("Unregistered receiver can be used for \e[34mft_transfer_call");
                    *os << "@False\n";
                } else {
                    Rustle::Logger().Info("Unregistered receiver can't be used for \e[34mft_transfer_call");
                    *os << "@True\n";
                }
            }

            return false;
        }
    };
}  // namespace

char UnregisteredReceiver::ID = 0;
static llvm::RegisterPass<UnregisteredReceiver> X("unregistered-receiver", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnregisteredReceiver()); });
