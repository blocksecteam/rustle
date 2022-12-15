/**
 * @file struct_member.cpp
 * @brief find struct member usages
 *
 */
#include "near_core.h"

#include <fstream>
#include <set>
#include <utility>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Regex.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {
    struct StructMember : public llvm::FunctionPass {
        static char ID;

      private:
        std::ifstream is;
        std::set<std::string> vars;

      public:
        StructMember() : FunctionPass(ID) { Rustle::readStructLog(is, vars); }
        ~StructMember() { is.close(); }

        bool runOnFunction(llvm::Function &F) override {
            using namespace llvm;
            if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                return false;
            if (Rustle::debug_print_function)
                Rustle::Logger().Debug("Checking function ", F.getName());

            bool found = false;
            for (BasicBlock &BB : F)
                for (Instruction &I : BB) {
                    if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                        continue;
                    auto result             = Rustle::usingStruct(&I, vars);
                    std::string const name  = result.first;
                    Rustle::Mode const mode = result.second;
                    if (name != "") {  // Found
                        Rustle::Logger().Info("Found struct <", name, "> used(",
                            mode == Rustle::Mode::RW ? "rw" : (mode == Rustle::Mode::Read ? "read" : (mode == Rustle::Mode::Write ? "write" : "unknown")), ") at ", I.getDebugLoc());
                        found = true;
                    }
                }
            if (!found && Rustle::debug_print_notfound)
                Rustle::Logger().Info("No struct member used found");

            return false;
        }
    };
}  // namespace

char StructMember::ID = 0;
static llvm::RegisterPass<StructMember> X("struct-member", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new StructMember()); });
