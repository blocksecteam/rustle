/**
 * @file div_before_mul.cpp
 * @brief find all division before multiplication that may lead to precision loss
 *
 * @ref https://github.com/crytic/slither/blob/master/slither/detectors/statements/divide_before_multiply.py
 *
 */
#include "near_core.h"

#include <cstring>

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
    struct DBM : public llvm::FunctionPass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        bool isMul(llvm::Instruction *I) {
            using namespace llvm;
            const char *opName = I->getOpcodeName();
            // mul, smul, umul, fmul
            return !strncmp(opName, "mul", 3) || !strncmp(opName + 1, "mul", 3);
        }
        bool isLlvmMul(llvm::Instruction *I) {
            using namespace llvm;

            // llvm.[mul, smul, umul, fmul]
            if (auto *callInst = dyn_cast<CallBase>(I)) {
                if (callInst->getCalledFunction())  // !!! important
                    if (Regex("llvm\\.[a-z]?mul\\.with\\.overflow\\.").match(callInst->getCalledFunction()->getName()))
                        return true;
            }

            return false;
        }

        /**
         * @brief
         *
         * @param I
         * @return true
         * @return false
         */
        bool isDiv(llvm::Instruction *I) {
            using namespace llvm;
            const char *opName = I->getOpcodeName();
            // sdiv, udiv, fdiv
            return !strncmp(opName + 1, "div", 3);
        }

      public:
        DBM() : FunctionPass(ID) {
            std::error_code EC;
            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.div-before-mul.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);
        }
        ~DBM() { os->close(); }

        /**
         * @brief
         *
         * @param F current function
         * @return true
         * @return false
         */
        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;

            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            bool foundOnFunc = false;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;

                    if (isDiv(&I)) {
                        std::set<Value *> set;
                        Rustle::simpleFindUsers(&I, set, true);
                        bool found = false;
                        for (auto &i : set) {
                            if (auto *inst = dyn_cast<Instruction>(i)) {
                                if (!inst->getDebugLoc().get() || Rustle::regexForLibLoc.match(inst->getDebugLoc().get()->getFilename()))
                                    continue;
                                if (isMul(inst) || isLlvmMul(inst)) {
                                    *os << F.getName() << "@" << I.getDebugLoc()->getFilename() << "@" << I.getDebugLoc().getLine() << "\n";
                                    found       = true;
                                    foundOnFunc = true;
                                    break;
                                }
                            }
                        }

                        if (found) {
                            std::vector<llvm::DebugLoc> mulLoc;
                            for (auto &i : set) {
                                if (auto *inst = dyn_cast<Instruction>(i)) {
                                    if (!inst->getDebugLoc().get() || Rustle::regexForLibLoc.match(inst->getDebugLoc().get()->getFilename()))
                                        continue;
                                    if (isMul(inst) || isLlvmMul(inst)) {
                                        mulLoc.push_back(inst->getDebugLoc());
                                    }
                                }
                            }
                            Rustle::Logger().Warning("Found div-before-mul, div at ", I.getDebugLoc(), ", mul at ", mulLoc);
                        }
                    }
                }
            }
            if (!foundOnFunc && Rustle::debug_print_notfound)
                Rustle::Logger().Info("Division before multiplication not found");
            return false;
        }
    };

}  // namespace

char DBM::ID = 0;
static llvm::RegisterPass<DBM> X("div-before-mul", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new DBM()); });
