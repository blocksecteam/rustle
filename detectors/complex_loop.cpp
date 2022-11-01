/**
 * @file call_inside_loop.cpp
 * @brief find all function calls inside a loop
 *
 */
#include "near_core.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Pass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fstream>
#include <numeric>
#include <vector>

namespace {
    struct HeavyLoop : public llvm::LoopPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        HeavyLoop() : LoopPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.complex-loop.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~HeavyLoop() { os->close(); }

        // int functionInstNum(llvm::Function *F) {
        //     using namespace llvm;
        //     if (!F)
        //         return 0;
        //     if (Rustle::regexForLibFunc.match(F->getName()))
        //         return 0;

        //     int count = 0;
        //     for (BasicBlock &BB : *F) {
        //         count += BB.sizeWithoutDebug();
        //     }
        //     return count;
        // }

        bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM) override {
            using namespace llvm;
            if (Rustle::regexForLibFunc.match(L->getHeader()->getParent()->getName()))
                return false;
            if (Rustle::regexForLibLoc.match(L->getStartLoc()->getFilename()))
                return false;
            if (Rustle::debug_print_function) {
                Rustle::Logger().Debug("Checking loop <", L->getName(), "> at ", L->getStartLoc());
            }

            auto blockVec     = L->getBlocksVector();
            auto loopInstSize = std::accumulate(blockVec.begin(), blockVec.end(), 0, [](auto num, BasicBlock *&bb) { return num + bb->sizeWithoutDebug(); });
            // int loopInstSize = 0;
            // for (BasicBlock *BB : L->getBlocks()) {
            //     for (Instruction &I : *BB) {
            //         if (auto callInst = dyn_cast<CallBase>(&I)) {
            //             loopInstSize += functionInstNum(callInst->getCalledFunction());
            //         } else
            //             loopInstSize++;
            //     }
            // }

            if (loopInstSize > Rustle::MIN_INST_NUM_FOR_LOOP) {
                Rustle::Logger().Warning("Loop with ", loopInstSize, " LLVM Instructions found at ", L->getStartLoc());
                *os << L->getHeader()->getParent()->getName() << "@" << L->getStartLoc()->getFilename() << "@" << L->getStartLoc().getLine() << "\n";
            }

            return false;
        }
    };
}  // namespace

char HeavyLoop::ID = 0;
static llvm::RegisterPass<HeavyLoop> X("complex-loop", "Pass to find all loops", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new HeavyLoop()); });
