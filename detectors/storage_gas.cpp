/**
 * @file storage_gas.cpp
 * @brief check if there is a check for storage fee after storage expansion
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
    struct StorageGas : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        StorageGas() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.storage-gas.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~StorageGas() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph CG(M);

            for (auto &F : M.functions()) {
                StringRef const funcFileName;

                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                bool hasStorageExpansion = false;

                for (BasicBlock &BB : F) {
                    bool const isPrivilege = false;
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

                        for (auto *caller : setCallers) {
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

char StorageGas::ID = 0;
static llvm::RegisterPass<StorageGas> X("storage-gas", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new StorageGas()); });
