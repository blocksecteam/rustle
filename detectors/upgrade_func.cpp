/**
 * @file upgrade_func.cpp
 * @brief check if there is an upgrade function in project
 *
 */
#include "near_core.h"

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
    struct UpdateFunc : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        UpdateFunc() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.upgrade-func.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UpdateFunc() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;
            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            bool call_promise_batch_action_deploy_contract = false, call_promise_batch_action_function_call = false;
            auto promise_batch_action_deploy_contract = Regex("promise_batch_action_deploy_contract"), promise_batch_action_function_call = Regex("promise_batch_action_function_call");
            StringRef funcFileName;

            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    if (Rustle::isInstCallFunc(&I, promise_batch_action_deploy_contract)) {
                        funcFileName                              = I.getDebugLoc().get()->getFilename();
                        call_promise_batch_action_deploy_contract = true;
                    }
                    if (Rustle::isInstCallFunc(&I, promise_batch_action_function_call)) {
                        call_promise_batch_action_function_call = true;
                    }
                }
            if (call_promise_batch_action_deploy_contract && call_promise_batch_action_function_call) {
                Rustle::Logger().Warning("Find upgrade in function ", F.getName(), " at ", funcFileName);
                *os << F.getName() << "@" << funcFileName << "\n";
            } else if (Rustle::debug_print_notfound)
                Rustle::Logger().Info("upgrade not found");

            return false;
        }
    };
}  // namespace

char UpdateFunc::ID = 0;
static llvm::RegisterPass<UpdateFunc> X("upgrade-func", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UpdateFunc()); });
