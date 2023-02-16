/**
 * @file timestamp.cpp
 * @brief warn of timestamp use which may be exploited by attacker
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
    struct TimeStamp : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        TimeStamp() : ModulePass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.timestamp.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~TimeStamp() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph const CG(M);

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
                            if (Regex("block_timestamp").match(callInst->getCalledFunction()->getName())) {
                                Rustle::Logger().Warning("timestamp used at ", &I.getDebugLoc());
                                *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                            }
                        }
                    }
            }
            return false;
        }
    };
}  // namespace

char TimeStamp::ID = 0;
static llvm::RegisterPass<TimeStamp> X("timestamp", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new TimeStamp()); });
