/**
 * @file call_inside_loop.cpp
 * @brief find all function calls inside a loop
 *
 */
#include "near_core.h"

#include <fstream>
#include <numeric>
#include <vector>

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct ComplexLoop : public llvm::LoopPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

      public:
        ComplexLoop() : LoopPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.complex-loop.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~ComplexLoop() { os->close(); }

        /**
         * @brief count functionInstNum with at most `depth` recursion
         *
         */
        int functionInstNum(llvm::Function *F, unsigned short depth = 2) {
            using namespace llvm;

            if (depth == 0)
                return 0;

            if (!F)
                return 0;
            if (Rustle::regexForLibFunc.match(F->getName()))
                return 0;

            int count = 0;
            for (BasicBlock &BB : *F) {
                for (Instruction &I : BB) {
                    if (auto *callInst = dyn_cast<CallBase>(&I)) {
                        count += functionInstNum(callInst->getCalledFunction(), depth - 1);
                    } else
                        count++;
                }
            }
            return count;
        }

        bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &lpm) override {
            using namespace llvm;
            if (Rustle::regexForLibFunc.match(L->getHeader()->getParent()->getName()))
                return false;
            if (Rustle::regexForLibLoc.match(L->getStartLoc()->getFilename()))
                return false;
            if (Rustle::debug_print_function) {
                Rustle::Logger().Debug("Checking loop <", L->getName(), "> at ", L->getStartLoc());
            }

            auto blockVec = L->getBlocksVector();

            // [option1] only consider current loop
            // auto loopInstSize = std::accumulate(blockVec.begin(), blockVec.end(), 0, [](auto num, BasicBlock *&bb) { return num + bb->sizeWithoutDebug(); });

            // [Option2] take sub function into consider
            int loopInstSize = 0;
            for (BasicBlock *BB : L->getBlocks()) {
                for (Instruction &I : *BB) {
                    if (auto *callInst = dyn_cast<CallBase>(&I)) {
                        loopInstSize += functionInstNum(callInst->getCalledFunction());
                    } else
                        loopInstSize++;
                }
            }

            if (loopInstSize > Rustle::MIN_INST_NUM_FOR_LOOP) {
                Rustle::Logger().Warning("Loop with ", loopInstSize, " LLVM Instructions found at ", L->getStartLoc());
                *os << L->getHeader()->getParent()->getName() << "@" << L->getStartLoc()->getFilename() << "@" << L->getStartLoc().getLine() << "\n";
            }

            return false;
        }
    };
}  // namespace

char ComplexLoop::ID = 0;
static llvm::RegisterPass<ComplexLoop> X("complex-loop", "Pass to find all loops", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new ComplexLoop()); });
