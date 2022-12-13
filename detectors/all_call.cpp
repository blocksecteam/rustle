/**
 * @file all_call.cpp
 * @brief find all function calls with caller and callee printed to tmp file
 *
 */
#include "near_core.h"

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
    struct AllCall : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        AllCall() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.all-call.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~AllCall() { os->close(); }

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

                    if (auto *callInst = dyn_cast<CallBase>(&I)) {
                        if (callInst->getCalledFunction())
                            *os << callInst->getCalledFunction()->getName() << "@" << callInst->getDebugLoc()->getFilename() << "@" << callInst->getDebugLoc()->getLine() << "\n";
                    }
                }

            return false;
        }
    };
}  // namespace

char AllCall::ID = 0;
static llvm::RegisterPass<AllCall> X("all-call", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new AllCall()); });
