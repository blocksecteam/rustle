/**
 * @file unsaved_changes.cpp
 * @brief check if all changes made to contract states have been saved
 *
 */
#include "near_core.h"

#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct UnsavedChanges : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        const llvm::Regex regexMapGet    = llvm::Regex("near_sdk[0-9]+collections[0-9]+"  // `get` in map collections
                                                       "(lookup_map[0-9]+LookupMap|tree_map[0-9]+TreeMap|unordered_map[0-9]+UnorderedMap|legacy_tree_map[0-9]+LegacyTreeMap)"
                                                          "\\$.+[0-9]+get[0-9]+");
        const llvm::Regex regexMapInsert = llvm::Regex("near_sdk[0-9]+collections[0-9]+"  // `insert` in map collections
                                                       "(lookup_map[0-9]+LookupMap|tree_map[0-9]+TreeMap|unordered_map[0-9]+UnorderedMap|legacy_tree_map[0-9]+LegacyTreeMap)"
                                                       "\\$.+[0-9]+insert[0-9]+");
        const llvm::Regex regexAllUnwrap = llvm::Regex("core[0-9]+option[0-9]+Option\\$.+[0-9]+"
                                                       "(unwrap|unwrap_or|unwrap_or_else|unwrap_or_default|unwrap_unchecked)[0-9]+");

      public:
        UnsavedChanges() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unsaved-changes.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnsavedChanges() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph const CG(M);

            for (auto &F : M.functions()) {

                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                for (BasicBlock &BB : F) {
                    for (Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        if (dyn_cast<CallBase>(&I) && Rustle::isInstCallFunc(&I, regexMapGet)) {

                            Value *returnValueOfGet = nullptr;
                            switch (dyn_cast<CallBase>(&I)->arg_size()) {
                                case 2: returnValueOfGet = &I; break;                                        // returnVal = call get(self, key)
                                case 3: returnValueOfGet = dyn_cast<CallBase>(&I)->getArgOperand(0); break;  // call get(returnVal, self, key)
                                default: break;
                            }
                            if (returnValueOfGet == nullptr) {  // skip other circumstances
                                continue;
                            }

                            std::set<Value *> usersOfGet;
                            Rustle::simpleFindUsers(returnValueOfGet,  // the first operand is the return value of `get`
                                usersOfGet, false, true);              // only find in current function

                            // Rustle::Logger().Debug(&I, "\n", dyn_cast<CallBase>(&I)->getArgOperand(0));

                            Value *unwrappedValue = nullptr;

                            for (auto *user : usersOfGet) {
                                if (dyn_cast<CallBase>(user) && Rustle::isInstCallFunc(dyn_cast<CallBase>(user), regexAllUnwrap)) {
                                    unwrappedValue = dyn_cast<CallBase>(user)->getArgOperand(0);
                                    break;
                                }
                            }

                            if (unwrappedValue == nullptr) {  // no unwrap
                                break;                        // skip this `get` instruction
                            }

                            std::set<Value *> usersOfUnwrappedValue;
                            Rustle::simpleFindUsers(unwrappedValue, usersOfUnwrappedValue, true);  // allow but restrict cross function

                            Instruction *instChangeValue = nullptr;

                            for (auto *user : usersOfUnwrappedValue) {
                                if (isa<StoreInst>(user)) {
                                    instChangeValue = dyn_cast<StoreInst>(user);
                                    break;
                                }
                            }

                            if (instChangeValue != nullptr) {
                                Instruction *instInsertValue = nullptr;
                                for (auto *user : usersOfUnwrappedValue) {
                                    if (dyn_cast<CallBase>(user) && Rustle::isInstCallFunc(dyn_cast<CallBase>(user), regexMapInsert)) {
                                        instInsertValue = dyn_cast<CallBase>(user);
                                        break;
                                    }
                                }

                                if (instInsertValue != nullptr) {
                                    Rustle::Logger().Info("Changes to map at ", I.getDebugLoc(), " have been saved at ", instInsertValue->getDebugLoc());
                                } else {
                                    Rustle::Logger().Warning("Changes to map at ", I.getDebugLoc(), " is unsaved");
                                    *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                                }
                            }
                        }
                    }
                }
            }
            return false;
        }
    };
}  // namespace

char UnsavedChanges::ID = 0;
static llvm::RegisterPass<UnsavedChanges> X("unsaved-changes", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnsavedChanges()); });
