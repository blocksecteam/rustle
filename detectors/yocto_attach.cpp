/**
 * @file yocto_attach.cpp
 * @brief check if there is an `assert_one_yocto` in privilege function
 *
 */
#include "near_core.h"

#include <fstream>
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
    struct YoctoAttach : public llvm::ModulePass {
        static char ID;

      private:
        llvm::raw_fd_ostream *os = nullptr;

        std::set<std::string> callbacks;

      public:
        YoctoAttach() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.yocto-attach.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);

            std::ifstream is;
            is.open(Rustle::callback_file);
            std::string callbackLine;
            while (is >> callbackLine) {
                callbacks.insert(callbackLine.substr(0, callbackLine.find('@')));
            }
            is.close();
        }
        ~YoctoAttach() { os->close(); }

        bool runOnModule(llvm::Module &M) override {
            using namespace llvm;

            CallGraph CG(M);

            for (auto &F : M.functions()) {
                StringRef funcFileName;

                if (!Rustle::debug_check_all_func && Rustle::regexForLibFunc.match(F.getName()))
                    continue;
                if (callbacks.count(F.getName().str()))  // skip callback functions
                    return false;
                if (Rustle::debug_print_function)
                    Rustle::Logger().Debug("Checking function ", F.getName());

                if (Rustle::isFuncPrivileged(&F)) {  // is privilege function, check assert_one_yocto usage
                    bool foundYoctoCheck = false;
                    for (BasicBlock &BB : F) {
                        for (Instruction &I : BB) {
                            if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                                continue;

                            if (funcFileName.empty())
                                if (I.getDebugLoc().get())
                                    funcFileName = I.getDebugLoc().get()->getFilename();

                            Regex const static regex_assert_yocto = Regex("near_sdk[0-9]+utils[0-9]+assert_one_yocto");
                            if (Rustle::isInstCallFuncRec(&I, CG, regex_assert_yocto)) {
                                Rustle::Logger().Info("Found yocto check for function ", F.getName(), " at ", I.getDebugLoc());

                                foundYoctoCheck = true;
                                break;
                            }
                        }
                    }

                    if (!foundYoctoCheck) {
                        Rustle::Logger().Warning("No yocto check for function ", F.getName());

                        *os << F.getName() << "@" << funcFileName << "\n";
                    }
                }
            }
            return false;
        }
    };
}  // namespace

char YoctoAttach::ID = 0;
static llvm::RegisterPass<YoctoAttach> X("yocto-attach", "", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &builder, llvm::legacy::PassManagerBase &PM) { PM.add(new YoctoAttach()); });
