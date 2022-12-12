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

        bool isInstPrivilege(llvm::Instruction *I, int const depth = 1) {
            using namespace llvm;
            Regex const static regex_predecessor = Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+predecessor_account_id");
            Regex const static regex_eq          = Regex("near_sdk\\.\\.types\\.\\.account_id\\.\\.AccountId.+core\\.\\.cmp\\.\\.PartialEq");

            if (depth < 0)
                return false;

            if (Rustle::isInstCallFunc(I, regex_predecessor)) {  // has called `predecessor_account_id`, check whether calls `PartialEq` in current function
                for (BasicBlock &BB : *(I->getFunction())) {
                    for (Instruction &i : BB) {
                        if (Rustle::isInstCallFunc(&i, regex_eq))
                            return true;
                    }
                }
            } else if (CallBase *callInst = dyn_cast<llvm::CallBase>(I)) {     // other call inst
                if (callInst->getCalledFunction()) {                           // returns null if this is an indirect function
                    for (BasicBlock &BB : *(callInst->getCalledFunction())) {  // check callee function
                        for (Instruction &i : BB) {
                            if (CallBase *callInst = dyn_cast<llvm::CallBase>(&i)) {
                                if (isInstPrivilege(&i, depth - 1)) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
            // not a call inst
            return false;
        }

      public:
        YoctoAttach() : ModulePass(ID) {
            std::error_code EC;

            os = new llvm::raw_fd_ostream(std::string(getenv("TMP_DIR")) + std::string("/.yocto-attach.tmp"), EC, llvm::sys::fs::OpenFlags::OF_Append);

            std::ifstream is;
            is.open(Rustle::callback_file);
            std::string callback_line;
            while (is >> callback_line) {
                callbacks.insert(callback_line.substr(0, callback_line.find('@')));
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

                for (BasicBlock &BB : F) {
                    bool isPrivilege = false;
                    for (Instruction &I : BB) {
                        if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                            continue;

                        if (isInstPrivilege(&I)) {
                            isPrivilege = true;
                            break;
                        }
                    }

                    if (isPrivilege) {  // is privilege function, check assert_one_yocto usage
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
            }
            return false;
        }
    };
}  // namespace

char YoctoAttach::ID = 0;
static llvm::RegisterPass<YoctoAttach> X("yocto-attach", "functions calling external function", false /* Only looks at CFG */, false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(llvm::PassManagerBuilder::EP_EarlyAsPossible, [](const llvm::PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) { PM.add(new YoctoAttach()); });
