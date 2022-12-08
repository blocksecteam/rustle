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
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct UnsavedChanges : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        const llvm::Regex regex_get =
            llvm::Regex("near_sdk[0-9]+collections[0-9]+(lookup_map[0-9]+LookupMap|tree_map[0-9]+TreeMap|unordered_map[0-9]+UnorderedMap)\\$.+[0-9]+get[0-9]+");  // `get` in map collections

      public:
        UnsavedChanges() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unsaved-changes.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnsavedChanges() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph CG(M);

            for (auto &F : M.functions()) {
                StringRef funcFileName;

                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                bool hasStorageExpansion = false;

                for (BasicBlock &BB : F) {
                    bool isPrivilege = false;
                    for (Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        auto const static regexStorageExpansion = Regex("near_sdk[0-9]+collections[0-9]+.+(insert|extend)");
                        if (Rustle::isInstCallFunc(&I, regexStorageExpansion)) {
                            hasStorageExpansion = true;
                            break;
                        }
                    }
                }

                if (hasStorageExpansion) {
                    *os << F.getName();

                    auto const static regexStorageUse = Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+storage_usage[0-9]+");

                    bool hasGasCheck = Rustle::isFuncCallFuncRec(&F, CG, regexStorageUse);  // find storage check in current function

                    if (!hasGasCheck) {  // find storage check in callers of current function
                        std::set<Function *> setCallers;
                        Rustle::findFunctionCallerRec(&F, setCallers, 2);  // only consider callers with limited depth, avoid false negative

                        for (auto caller : setCallers) {
                            hasGasCheck |= Rustle::isFuncCallFuncRec(caller, CG, regexStorageUse);
                            if (hasGasCheck)
                                break;
                        }
                    }

                    if (!hasGasCheck) {
                        Rustle::Logger().Warning("Lack of storage check in function ", F.getName());
                        *os << "@False\n";
                    } else {
                        Rustle::Logger().Warning("Find storage check in function ", F.getName());
                        *os << "@True\n";
                    }
                }
            }
            return false;
        }
    };
}  // namespace

char UnsavedChanges::ID = 0;
static llvm::RegisterPass<UnsavedChanges> X("unsaved-changes", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnsavedChanges()); });
