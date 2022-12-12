/**
 * @file promise_result.cpp
 * @brief find all PromiseResult uses
 *
 */
#include "near_core.h"

#include <fstream>
#include <set>
#include <utility>

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
    struct PromiseResult : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        PromiseResult() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.promise-result.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~PromiseResult() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;
            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;
                    // if (auto CI = dyn_cast<CastInst>(&I)) {  // for offset > 0
                    //     if (Regex(".+PromiseResult.+").match(Rustle::printToString(CI->getSrcTy()))) {
                    if (Rustle::isInstCallFunc(&I, Rustle::regexPromiseResult)) {
                        Rustle::Logger().Info("PromiseResult use at ", &I.getDebugLoc());
                        *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                    }
                    //     }
                    // }
                }
            return false;
        }
    };
}  // namespace

char PromiseResult::ID = 0;
static llvm::RegisterPass<PromiseResult> X("promise-result", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new PromiseResult()); });
