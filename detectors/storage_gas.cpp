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
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct YoctoAttach : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        std::set<std::string> callbacks;

      public:
        YoctoAttach() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.storage-gas.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);

            std::ifstream is;
            is.open(Rustle::callback_file);
            std::string callback_line;
            while (is >> callback_line) {
                callbacks.insert(callback_line.substr(0, callback_line.find('@')));
            }
            is.close();
        }
        ~YoctoAttach() { os->close(); }

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

                        auto regexStorageExpansion = Regex("near_sdk[0-9]+collections[0-9]+.+(insert|extend)");
                        if (Rustle::isInstCallFuncRec(&I, CG, regexStorageExpansion)) {
                            hasStorageExpansion = true;
                            break;
                        }
                    }
                }

                if (hasStorageExpansion) {
                    auto regexStorageUse = Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+storage_usage[0-9]+");

                    // Rustle::Logger().Debug(CG[&F]->size());

                    if (!Rustle::isFuncCallFuncRec(&F, CG, regexStorageUse)) {
                        Rustle::Logger().Warning("Lack of storage check in function ", F.getName());
                    } else {
                        Rustle::Logger().Warning("Find check in function ", F.getName());
                    }
                }
            }
            return false;
        }
    };
}  // namespace

char YoctoAttach::ID = 0;
static llvm::RegisterPass<YoctoAttach> X("storage-gas", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new YoctoAttach()); });
