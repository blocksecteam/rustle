/**
 * @file ext_call_trait.cpp
 * @brief find all trait functions used as external calls' traits
 *
 */
#include "near_core.h"

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
    struct ExtCallTrait : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        ExtCallTrait() : ModulePass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(Rustle::ext_call_trait_file, EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~ExtCallTrait() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph const CG(M);

            for (auto &F : M.functions()) {
                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                for (llvm::BasicBlock &BB : F)
                    for (llvm::Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(&I)) {
                            if (!callInst->getCalledFunction())
                                continue;
                            if (Rustle::regexExtCall.match(callInst->getCalledFunction()->getName())) {
                                *os << F.getName() << "\n";
                                return false;
                            }
                        }
                    }
            }
            return false;
        }
    };
}  // namespace

char ExtCallTrait::ID = 0;
static llvm::RegisterPass<ExtCallTrait> X("ext-call-trait", "function tagged with #[ext_contract()]", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new ExtCallTrait()); });
