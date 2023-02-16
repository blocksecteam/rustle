/**
 * @file transfer.cpp
 * @brief hint transfer behaviors
 *
 */
#include "near_core.h"

#include <set>
#include <vector>

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
    struct Transfer : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        Transfer() : ModulePass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.transfer.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~Transfer() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph CG(M);

            for (auto &F : M.functions()) {
                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                bool found = false;

                for (BasicBlock &BB : F) {
                    for (Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        if (Rustle::isInstCallFuncRec(&I, CG, Rustle::regexPromiseTransfer)) {
                            found = true;
                            Rustle::Logger().Warning("Found promise transfer at ", &I.getDebugLoc());
                            *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                        } else if (Rustle::isInstCallFuncRec(&I, CG, Rustle::regexNep141Transfer)) {
                            found = true;
                            Rustle::Logger().Warning("Found NEP-141 transfer at ", &I.getDebugLoc());
                            *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                        }
                    }
                }

                if (!found && Rustle::debug_print_notfound)
                    Rustle::Logger().Info("transfer or invoke not found");
            }

            return false;
        }
    };

}  // namespace

char Transfer::ID = 0;
static llvm::RegisterPass<Transfer> X("transfer", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new Transfer()); });
