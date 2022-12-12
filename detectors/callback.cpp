/**
 * @file callback.cpp
 * @brief find all callback funcs by identifying name and promise_result use
 *
 */
#include "near_core.h"

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
    struct Callback : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        Callback() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(Rustle::callback_file, EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~Callback() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;

            StringRef funcFileName;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            for (llvm::BasicBlock &BB : F)
                for (llvm::Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    if (funcFileName.empty()) {  // only once
                        if (I.getDebugLoc().get())
                            funcFileName = I.getDebugLoc().get()->getFilename();
                        // if (F.getName().contains("callback")) {
                        //     *os << F.getName() << "@" << funcFileName << "\n";
                        //     return false;
                        // }
                    }

                    if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(&I)) {
                        if (!callInst->getCalledFunction())
                            continue;
                        if (Rustle::regexPromiseResult.match(callInst->getCalledFunction()->getName())) {
                            *os << F.getName() << "@" << funcFileName << "\n";
                            return false;
                        }
                    }
                }

            return false;
        }
    };
}  // namespace

char Callback::ID = 0;
static llvm::RegisterPass<Callback> X("callback", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new Callback()); });
