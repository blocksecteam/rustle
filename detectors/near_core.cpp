#include "near_core.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"

namespace Rustle {
    void simpleFindUsers(llvm::Value *value, std::set<llvm::Value *> &set, bool restrictCrossFunction, bool disableCrossFunction) {
        using namespace llvm;

        auto *const VALUE_TYPE = value->getType();
        if (!VALUE_TYPE || VALUE_TYPE->isLabelTy())
            return;

        if (set.find(value) != set.end())
            return;

        set.insert(value);

        if (auto *callInst = dyn_cast<CallBase>(value)) {
            for (unsigned i = 0; i < callInst->arg_size(); i++) {
                if (!disableCrossFunction && callInst->getCalledFunction() && (!restrictCrossFunction || set.count(callInst->getArgOperand(i))))
                    simpleFindUsers(callInst->getCalledFunction()->getArg(i), set, restrictCrossFunction, disableCrossFunction);  // cross-function
                if (callInst->getCalledFunction() && callInst->getCalledFunction()->getName().contains("core..convert..Into") && callInst->arg_size() >= 2 && set.count(callInst->getArgOperand(1))) {
                    simpleFindUsers(callInst->getArgOperand(0), set, restrictCrossFunction, disableCrossFunction);  // add `xxx.into()` as user of `xxx`
                }
            }
        } else if (auto *stInst = dyn_cast<StoreInst>(value)) {
            simpleFindUsers(stInst->getPointerOperand(), set, restrictCrossFunction, disableCrossFunction);
        }

        for (auto *U : value->users()) {
            simpleFindUsers(U, set, restrictCrossFunction, disableCrossFunction);
        }
    }

    void findUsers(llvm::Value *value, std::set<llvm::Value *> &set, const int GEP_OFFSET, int depth) {
        using namespace llvm;
        if (depth <= 0)
            return;
        if (set.count(value) == 1)
            return;

        auto *const VALUE_TYPE = value->getType();

        if (!VALUE_TYPE || VALUE_TYPE->isLabelTy())
            return;

        set.insert(value);

        if (auto *unaInst = dyn_cast<UnaryInstruction>(value)) {  // alloca, cast, extract, freeze, load, unaryOp, VAA
            if (GEP_OFFSET != -1) {
                if (auto *bcInst = dyn_cast<BitCastInst>(value)) {
                    std::string const resType = printToString(bcInst->getDestTy()), srcType = printToString(bcInst->getSrcTy());
                    if (GEP_OFFSET != 0 && (StringRef(resType).contains("solana_program::pubkey::Pubkey") && StringRef(srcType).contains("solana_program::account_info::AccountInfo") ||
                                               (StringRef(resType).contains("anchor_lang::prelude::Pubkey") && StringRef(srcType).contains("anchor_lang::prelude::AccountInfo"))))
                        return;  // BitCast Not allowed for Offset!=0
                }
            }
            for (auto *U : unaInst->users())
                findUsers(U, set, GEP_OFFSET, depth - 1);
        } else if (auto *binOpInst = dyn_cast<BinaryOperator>(value)) {
            for (auto *U : binOpInst->users())
                findUsers(U, set, GEP_OFFSET, depth - 1);
        } else if (auto *gepInst = dyn_cast<GetElementPtrInst>(value)) {
            bool goFurther = GEP_OFFSET == -1;

            if (!goFurther) {
                std::string const resType = printToString(gepInst->getResultElementType()), srcType = printToString(gepInst->getSourceElementType());

                switch (dyn_cast<ConstantInt>(gepInst->getOperand(gepInst->getNumOperands() - 1))->getZExtValue()) {
                    // more cases can be set
                    case 3:  // owner field
                        if (GEP_OFFSET == 3 && (StringRef(resType).contains("solana_program::pubkey::Pubkey") && StringRef(srcType).contains("solana_program::account_info::AccountInfo")) ||
                            (StringRef(resType).contains("anchor_lang::prelude::Pubkey") && StringRef(srcType).contains("anchor_lang::prelude::AccountInfo")))
                            goFurther = true;
                        break;

                    default: goFurther = true; break;
                }
            }

            if (goFurther)
                for (auto *U : gepInst->users())
                    findUsers(U, set, GEP_OFFSET, depth - 1);
        } else if (auto *stInst = dyn_cast<StoreInst>(value)) {
            findUsers(stInst->getPointerOperand(), set, GEP_OFFSET, depth - 1);
        } else if (auto *callInst = dyn_cast<CallBase>(value)) {
            if (callInst->getCalledFunction() && callInst->getCalledFunction()->getName().startswith("llvm.memcpy") && callInst->getArgOperand(0)) {
                for (auto *U : callInst->getArgOperand(0)->users())  // memcpy target
                    findUsers(U, set, GEP_OFFSET, depth - 1);
                if (auto *cstInst = dyn_cast<CastInst>(callInst->getArgOperand(0))) {  // memcpy target pointer
                    if (cstInst->getOperand(0)) {
                        for (auto *U : cstInst->getOperand(0)->users())
                            findUsers(U, set, GEP_OFFSET, depth - 1);
                    }
                }
            } else if (callInst->getCalledFunction() && callInst->getCalledFunction()->getName().contains("core..cmp")) {
                // ends here, skip compare function
            } else if (GEP_OFFSET != -1 && callInst->getCalledFunction() && Regex("solana_program[0-9]+program_pack[0-9]+Pack[0-9]+unpack").match(callInst->getCalledFunction()->getName())) {
                // ends here, skip unpack result for owner-check
            } else if (callInst->getCalledFunction() && callInst->getCalledFunction()->getName().contains("core..convert..Into") && callInst->arg_size() >= 2 &&
                       set.count(callInst->getArgOperand(1))) {
                findUsers(callInst->getArgOperand(0), set, GEP_OFFSET, depth - 1);  // add `xxx.into()` as user of `xxx`
            } else {
                // for (unsigned i = 0; i < CallInst->getCalledFunction()->arg_size(); i++)
                //     findUsers(CallInst->getCalledFunction()->getArg(i), set, GEPOffset, depth - 1);  // cross function
                for (unsigned i = 0; i < callInst->arg_size(); i++)
                    findUsers(callInst->getArgOperand(i), set, GEP_OFFSET, depth - 1);  // function args
                for (auto *U : callInst->users())
                    findUsers(U, set, GEP_OFFSET, depth - 1);
            }
        } else if (auto *inst = dyn_cast<Instruction>(value)) {
            // ignore other instructions
        } else {
            for (auto *U : value->users()) {
                if (set.find(U) != set.end())
                    continue;
                findUsers(U, set);
            }
        }
    }

    bool isFuncPrivileged(llvm::Function *F, int const depth) {
        using namespace llvm;

        Regex const static regex_predecessor = Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+predecessor_account_id");
        Regex const static regex_eq          = Regex("near_sdk\\.\\.types\\.\\.account_id\\.\\.AccountId.+core\\.\\.cmp\\.\\.PartialEq");

        if (F == nullptr)
            return false;

        if (depth < 0)
            return false;

        for (BasicBlock &BB : *F) {
            for (Instruction &I : BB) {
                if (!I.getDebugLoc().get() || Rustle::regexForLibLoc.match(I.getDebugLoc().get()->getFilename()))
                    continue;

                if (Rustle::isInstCallFunc(&I, regex_predecessor)) {
                    // has called `predecessor_account_id`, check whether calls `PartialEq` in current function
                    for (BasicBlock &BB : *F) {
                        for (Instruction &i : BB) {
                            if (Rustle::isInstCallFunc(&i, regex_eq))
                                return true;
                        }
                    }
                } else if (CallBase *callInst = dyn_cast<llvm::CallBase>(&I)) {            // other call inst
                    if (callInst->getCalledFunction()) {                                   // indirect function invocation returns null
                        if (isFuncPrivileged(callInst->getCalledFunction(), depth - 1)) {  // check callee function
                            return true;
                        }
                    }
                }
            }
        }
        // not a call inst
        return false;
    }

    bool isInstCallFunc(llvm::Instruction *I, llvm::Regex const &regex) {
        if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(I)) {
            if (callInst->getCalledFunction()) {
                llvm::StringRef const calleeName = callInst->getCalledFunction()->getName();
                if (regex.match(calleeName)) {
                    return true;
                }
                if (debug_print_derive_pack && calleeName == "llvm.dbg.declare" && callInst->getArgOperand(0)) {
                    std::string const argMetadata = printToString(llvm::dyn_cast<llvm::MetadataAsValue>(callInst->getArgOperand(0))->getMetadata());
                    if (regex.match(llvm::StringRef(argMetadata)))
                        return true;
                }
            }
        }
        return false;
    }

    namespace {  // Hide this func
        bool isFuncCallFuncRec(llvm::CallGraphNode *node, llvm::Regex const &regex, std::vector<llvm::CallGraphNode *> &callStack) {
            if (!node)
                return false;

            // Find in current level
            for (auto callee : *node) {
                if (callee.second->getFunction() && regex.match(callee.second->getFunction()->getName()))
                    return true;
            }

            // Find in next level
            for (auto callee : *node) {
                if (callee.second == node || find(callStack.begin(), callStack.end(), node) != callStack.end())  // important, avoid loop
                    continue;

                callStack.push_back(callee.second);
                if (isFuncCallFuncRec(callee.second, regex, callStack))
                    return true;
                callStack.pop_back();
            }
            return false;
        }
    }  // namespace

    // Recursive version
    bool isInstCallFuncRec(llvm::Instruction *I, llvm::CallGraph &CG, llvm::Regex const &regex) {
        if (isInstCallFunc(I, regex))
            return true;

        if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(I)) {
            if (callInst->getCalledFunction()) {
                if (llvm::Regex("llvm").match(callInst->getCalledFunction()->getName()))
                    return false;

                std::vector<llvm::CallGraphNode *> callStack;
                callStack.push_back(CG[callInst->getCalledFunction()]);
                return isFuncCallFuncRec(CG[callInst->getCalledFunction()], regex, callStack);
            }
        }
        return false;
    }

    // Recursive version
    bool isFuncCallFuncRec(llvm::Function *F, llvm::CallGraph &CG, llvm::Regex const &regex) {
        if (llvm::Regex("llvm").match(F->getName()))
            return false;

        std::vector<llvm::CallGraphNode *> callStack;
        callStack.push_back(CG[F]);
        return isFuncCallFuncRec(CG[F], regex, callStack);
    }

    void findFunctionCallerRec(llvm::Function *F, std::set<llvm::Function *> &set, int depth) {
        using namespace llvm;
        if (depth <= 0)
            return;

        for (auto &ui : F->uses()) {
            if (auto *callInst = dyn_cast<CallBase>(ui.getUser())) {
                // Function *caller = callInst->getParent()->getParent();
                Function *caller = callInst->getFunction();

                if (caller && !set.count(caller)) {
                    set.insert(caller);
                    findFunctionCallerRec(caller, set, depth - 1);
                }
            }
        }
    }

    namespace {  // Hide this func
        /**
         * @brief Get the Mode of user
         *
         * @param user
         * @param prevUser
         * @return int based on Mode enum
         */
        int getMode(llvm::Value *user, llvm::Value *prevUser) {
            using namespace llvm;

            // outs() << "user: " << *user << "\n";
            if (isa<LoadInst>(user)) {
                // outs() << "load\n";
                return Mode::Read;
            } else if (isa<StoreInst>(user)) {
                // outs() << "store\n";
                return Mode::Write;
            } else if (auto *memcpyInst = dyn_cast<MemCpyInst>(user)) {
                if (memcpyInst->getRawDest() == prevUser)
                    return Mode::Write;
                else if (memcpyInst->getRawSource() == prevUser)
                    return Mode::Read;
                else
                    return Mode::Unknown;
            } else if (auto *callInst = dyn_cast<CallBase>(user)) {
                // _ZN8near_sdk11collections13unordered_set21UnorderedSet$LT$T$GT$6insert17hf8c99f299d0290bbE
                if (callInst->getCalledFunction()) {
                    if (Regex("near_sdk[0-9]+collections([0-9a-zA-Z]|_+)+(\\$|[0-9a-zA-Z])+(insert_raw|remove_raw|remove|insert|clear|iter|extend|as_vector|keys|values|range|push|pop|replace|swap_"
                              "remove)")
                            .match(callInst->getCalledFunction()->getName())) {
                        return Mode::Write;
                    } else if (Regex("near_sdk[0-9]+collections([0-9a-zA-Z]|_+)+(\\$|[0-9a-zA-Z])+(len|is_empty|new|contains|to_vec|get|floor_key|ceil_key)")
                                   .match(callInst->getCalledFunction()->getName())) {
                        return Mode::Write;
                    } else if (Regex("core[0-9]+ptr[0-9]+drop_in_place").match(callInst->getCalledFunction()->getName())) {
                        return Mode::Read;
                    } else if (callInst->getCalledFunction()->getName().contains("clone")) {
                        return Mode::Read;
                    } else {
                        // outs() << "call\n";
                        // outs() << "hasArgument(prevUser): " << callInst->hasArgument(prevUser) << "\n";

                        // outs() << "callInst->arg_size(): " << callInst->arg_size() << "\n";

                        // outs() << "args_op: ";
                        int userNo = -1;
                        for (auto i = 0; i < callInst->arg_size(); i++) {
                            if (callInst->getArgOperand(i) == prevUser)
                                userNo = i;
                            // outs() << *callInst->getArgOperand(i) << " | ";
                        }
                        // outs() << "\n";

                        // outs() << "user_no: " << user_no << "\n";
                        if (userNo == -1) {  // user not found in args
                            // outs() << "[ERR] user not found in args\n";
                            return Mode::Unknown;
                        }

                        // outs() << "args: ";
                        // for (auto &u : callInst->args())
                        // outs() << u->getName() << " ";
                        // outs() << "\n";

                        // outs() << "callInst->getCalledFunction()->arg_size(): " << callInst->getCalledFunction()->arg_size() << "\n";

                        // outs() << "arg passed: ";
                        // for (auto &i : callInst->getCalledFunction()->args())
                        // outs() << i.getName() << " | ";
                        // outs() << "\n\n";

                        prevUser = user;
                        user     = callInst->getCalledFunction()->getArg(userNo);
                        return getMode(user, prevUser);
                        // outs() << "getCalledOperand: " << callInst->getCalledOperand()[user_no] << "\n";
                    }
                } else
                    return Mode::Unknown;
            } else {  // Other instructions
                // outs() << "user->getNumUses(): " << user->getNumUses() << "\n";
                if (user->getNumUses() > 0) {
                    prevUser   = user;
                    int result = Mode::Unknown;
                    for (auto *i : user->users()) {
                        user = dyn_cast<Instruction>(i);
                        result |= getMode(user, prevUser);
                    }
                    return result;
                } else {
                    // outs() << "[ERR] user->getNumUses() == 0\n";
                    return Mode::Unknown;
                }
            }
        }
    }  // namespace

    std::pair<std::pair<std::string, unsigned>, Mode> usingStructMember(llvm::Instruction *I, std::set<std::pair<std::string, unsigned>> &vars) {
        using namespace llvm;

        Value *castValue = nullptr;               // pointer to llvm::Value of member that has been cast from struct pointer, also serves as a flag
        std::pair<std::string, unsigned> target;  // pair of struct name and member offset

        if (auto *gepInst = dyn_cast<GetElementPtrInst>(I)) {  // for offset > 0
            for (auto i : vars) {
                std::string const srcMeta = printToString(gepInst->getSourceElementType());
                if (srcMeta.find('=') == std::string::npos)
                    continue;
                std::string structName = srcMeta.substr(1, srcMeta.find('=') - 2);
                structName.erase(std::remove(structName.begin(), structName.end(), '"'), structName.end());

                /* Control exact match */
                // if (structName.find(i.first) != std::string::npos && dyn_cast<ConstantInt>(GEPInst->getOperand(GEPInst->getNumOperands() - 1))->equalsInt(i.second)) {
                if (structName == i.first && dyn_cast<ConstantInt>(gepInst->getOperand(gepInst->getNumOperands() - 1))->equalsInt(i.second)) {
                    castValue = dyn_cast<Value>(I);
                    target    = i;
                    break;
                }
            }
        } else if (auto *CI = dyn_cast<CastInst>(I)) {  // for offset = 0
            for (auto i : vars) {
                if (i.second != 0)  // skip offset > 0
                    continue;

                std::string const srcMeta = printToString(CI->getSrcTy());
                if (srcMeta.find(i.first) == std::string::npos)
                    continue;

                std::string structName = srcMeta.substr(1, srcMeta.size() - 2);  // remove '%' and '*'

                if (structName.at(structName.size() - 1) == '*')  // make sure only 1 level deref
                    continue;

                /* Control exact match */
                // if (structName.find(i.first) != std::string::npos && i.second == 0) {
                if (structName == i.first && i.second == 0) {
                    castValue = dyn_cast<Value>(I);
                    target    = i;
                    break;
                }
            }
        }

        if (!castValue)  // Not found
            return std::make_pair(std::make_pair("", 0), Mode::Unknown);

        // outs() << "I: " << *I << "\n";
        if (!castValue->hasOneUser()) {
            return std::make_pair(target, Mode::Unknown);
        }

        if (I->getNumUses() > 0) {
            Value *user     = dyn_cast<Value>(*I->user_begin());
            Value *prevUser = I;
            int const mode  = getMode(user, prevUser);
            switch (mode) {
                case Mode::RW:
                    return std::make_pair(target, Mode::Write);
                    // return std::make_pair(target, Mode::RW);
                case Mode::Write: return std::make_pair(target, Mode::Write);
                case Mode::Read: return std::make_pair(target, Mode::Read);
                default: return std::make_pair(target, Mode::Unknown);
            }
        } else
            return std::make_pair(target, Mode::Unknown);
    }

    void readStructMember(std::ifstream &is, std::set<std::pair<std::string, unsigned>> &vars) {
        if (Rustle::debug_print_tmp)
            llvm::outs() << "[*] Reading variable list\n";
        is.open("struct_member.conf");
        std::string structName;
        unsigned offset;
        while (is >> structName) {
            if (structName[0] == '#') {
                is.ignore(200, '\n');
                continue;
            } else
                is >> offset;
            vars.insert(std::make_pair(structName, offset));
        }
        if (Rustle::debug_print_tmp) {
            llvm::outs() << "[*] Get " << vars.size() << " variables\n";
            for (auto i : vars)
                llvm::outs() << i.first << " " << i.second << "\n";
        }
    }

    std::pair<std::string, Mode> usingStruct(llvm::Instruction *I, std::set<std::string> &structs) {
        using namespace llvm;

        Value *castValue = nullptr;  // pointer to llvm::Value of member that has been cast from struct pointer
                                     // also serves as a flag
        std::string target;          // pair of struct name and member offset

        if (auto *gepInst = dyn_cast<GetElementPtrInst>(I)) {  // for offset > 0
            std::string const srcMeta = printToString(gepInst->getSourceElementType());
            if (srcMeta.find('=') == std::string::npos)
                ;
            else if (srcMeta.size() >= 2) {
                std::string structName = srcMeta.substr(1, srcMeta.find('=') - 2);
                structName.erase(std::remove(structName.begin(), structName.end(), '"'), structName.end());
                for (auto i : structs) {
                    /* Control exact match */
                    // if (structName.find(i.first) != std::string::npos && dyn_cast<ConstantInt>(GEPInst->getOperand(GEPInst->getNumOperands() - 1))->equalsInt(i.second)) {
                    if (structName == i) {
                        castValue = dyn_cast<Value>(I);
                        target    = i;
                        break;
                    }
                }
            }
        } else if (auto *CI = dyn_cast<CastInst>(I)) {  // for offset = 0
            std::string srcMeta = printToString(CI->getSrcTy());
            if (srcMeta[0] == '%' && srcMeta[srcMeta.size() - 1] == '*') {
                std::string structName = srcMeta.substr(1, srcMeta.size() - 2);  // remove '%' and '*'
                if (structName.at(structName.size() - 1) == '*')                 // make sure only 1 level deref
                    ;
                else
                    for (auto i : structs) {
                        if (srcMeta.find(i) == std::string::npos)
                            continue;

                        /* Control exact match */
                        if (structName == i) {
                            castValue = dyn_cast<Value>(I);
                            target    = i;
                            break;
                        }
                    }
            }
        }

        if (!castValue)  // Not found
            return std::make_pair("", Mode::Unknown);

        // outs() << "I: " << *I << "\n";
        if (!castValue->hasOneUser()) {
            return std::make_pair(target, Mode::Unknown);
        }

        if (I->getNumUses() > 0) {
            Value *user     = dyn_cast<Value>(*I->user_begin());
            Value *prevUser = I;
            int const mode  = getMode(user, prevUser);
            switch (mode) {
                case Mode::RW:
                    return std::make_pair(target, Mode::Write);
                    // return std::make_pair(target, Mode::RW);
                case Mode::Write: return std::make_pair(target, Mode::Write);
                case Mode::Read: return std::make_pair(target, Mode::Read);
                default: return std::make_pair(target, Mode::Unknown);
            }
        } else
            return std::make_pair(target, Mode::Unknown);
    }

    void readStructLog(std::ifstream &is, std::set<std::string> &structs) {
        if (Rustle::debug_print_tmp)
            llvm::outs() << "[*] Reading variable list\n";
        is.open(std::string(getenv("TMP_DIR")) + std::string("/.structs.tmp"));
        std::string structName;
        unsigned offset;
        while (is >> structName) {
            if (structName[0] == '#') {
                is.ignore(200, '\n');
                continue;
            } else
                structs.insert(structName);
        }
        if (Rustle::debug_print_tmp) {
            llvm::outs() << "[*] Get " << structs.size() << " struct\n";
            for (auto i : structs)
                llvm::outs() << i << "\n";
        }
    }

}  // namespace Rustle
