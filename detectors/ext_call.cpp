/**
 * @file ext_call.cpp
 * @brief find functions that call external function
 *
 */
#include "near_core.h"

#include <fstream>
#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct ExtCall : public llvm::ModulePass {
        static char ID;

        std::vector<std::string> ext_call_traits;

      public:
        ExtCall() : ModulePass(ID) {
            std::ifstream is;
            is.open(Rustle::ext_call_trait_file);
            std::string ext_call_trait;
            while (is >> ext_call_trait) {
                ext_call_traits.push_back(ext_call_trait);
            }
            is.close();
        }
        ~ExtCall() {}

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph CG(M);

            for (auto &F : M.functions()) {
                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                for (BasicBlock &BB : F)
                    for (Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        if (CallBase *callInst = dyn_cast<CallBase>(&I)) {
                            if (!callInst->getCalledFunction())
                                continue;
                            for (auto ext_call_trait : ext_call_traits)
                                if (Rustle::isInstCallFunc(&I, Regex(ext_call_trait))) {
                                    Rustle::Logger().Info("Function <", F.getName(), "> calls\n\t<", callInst->getCalledFunction()->getName(), ">\n\tat ", I.getDebugLoc());
                                }
                        }
                    }
            }
            return false;
        }
    };
}  // namespace

char ExtCall::ID = 0;
static llvm::RegisterPass<ExtCall> X("ext-call", "functions calling external function", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new ExtCall()); });
