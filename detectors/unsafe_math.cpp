/**
 * @file unsafe_math.cpp
 * @brief find all unchecked arithmetic operations
 *
 */
#include "near_core.h"

#include <cstring>
#include <string>

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
    struct UnsafeMath : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        bool useSafeMath(llvm::Instruction *I) {
            using namespace llvm;

            if (auto *callInst = dyn_cast<CallBase>(I)) {
                if (callInst->getCalledFunction())  // !!! important
                    if (Regex("llvm\\..+\\.with\\.overflow\\.").match(callInst->getCalledFunction()->getName()) || Regex("core.+num.+checked_").match(callInst->getCalledFunction()->getName())) {
                        return true;
                    }
            }
            return false;
        }

      public:
        UnsafeMath() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.unsafe-math.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~UnsafeMath() { os->close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    auto *debugLoc = I.getDebugLoc().get();
                    if (!debugLoc || Rustle::regexForLibLoc.match(debugLoc->getFilename()))
                        continue;

                    // // Option 1: positive detection
                    // useSafeMath(&I);

                    // Option 2: negative detection with variable name sense
                    const char *opName = I.getOpcodeName();
                    if ((!strncmp(opName, "mul", 3) || !strncmp(opName + 1, "mul", 3) || !strncmp(opName, "add", 3) || !strncmp(opName + 1, "add", 3) || !strncmp(opName, "sub", 3) ||
                            !strncmp(opName + 1, "sub", 3)) &&
                        opName[0] != 'f') {  // exclude float
                        bool foundUserDefVar = false;
                        // check result
                        if (auto *b = dyn_cast<BinaryOperator>(&I))
                            if (b->getName() != "")
                                foundUserDefVar = true;
                        // check operands
                        for (int i = 0; i < I.getNumOperands(); i++) {
                            if (I.getOperand(i)->getName() != "")
                                foundUserDefVar = true;
                        }
                        if (foundUserDefVar) {
                            Rustle::Logger().Warning("unsafe math use at ", &I.getDebugLoc());

                            *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                        }
                    }

                    // // Option 3: negative detection directly
                    // const char *opName = I.getOpcodeName();
                    // if (!strncmp(opName, "mul", 3) || !strncmp(opName + 1, "mul", 3) || !strncmp(opName, "add", 3) || !strncmp(opName + 1, "add", 3) || !strncmp(opName, "sub", 3) ||
                    //     !strncmp(opName + 1, "sub", 3)) {
                    //     outs() << "\e[33m[!] Found unsafe math at \e[34m";
                    //     I.getDebugLoc().print(outs());
                    //     outs() << "\e[0m\n";
                    // }
                }
            }
            return false;
        }
    };

}  // namespace

char UnsafeMath::ID = 0;
static llvm::RegisterPass<UnsafeMath> X("unsafe-math", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new UnsafeMath()); });
