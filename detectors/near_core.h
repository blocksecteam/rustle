#pragma once

#include <cstring>
#include <fstream>
#include <set>
#include <utility>
#include <vector>

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Pass.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace Rustle {
    // tool config
    std::string ext_call_trait_file = std::string(getenv("TMP_DIR")) + std::string("/.ext-call-trait.tmp");
    std::string callback_file       = std::string(getenv("TMP_DIR")) + std::string("/.callback.tmp");
    const int MIN_INST_NUM_FOR_LOOP = 100;

    bool debug_check_all_func = true;

    bool debug_print_function = false;
    bool debug_print_tmp      = false;
    bool debug_print_notfound = debug_print_function && false;

    bool debug_print_derive_pack = false;  // show pack/unpack from `#[derive(Serialize, Deserialize)]`

    // built-in regex
    auto regexForLibFunc = llvm::Regex("(^/cargo)|(^/rustc)"
                                       "|(_ZN\\d+(core|std|alloc|num_traits|solana_program|byteorder|hex|bytemuck|borsh|enumflags2|safe_transmute|thiserror|byteorder)([0-9]+|\\.\\.)[a-zA-Z]+)"
                                       "|(serde\\.\\.de\\.\\.Deserialize)");
    auto regexForLibLoc  = llvm::Regex("(^/rustc)|(^/cargo)|(^/root/.cargo)|(^/home/.+/.cargo)|(^$)");

    auto regexExtCall         = llvm::Regex("(.+near_sdk[0-9]+promise[0-9]+Promise[0-9]+function_call(_weight)?[0-9]+)");
    auto regexPromiseTransfer = llvm::Regex("near_sdk[0-9]+promise[0-9]+Promise[0-9]+transfer[0-9]+");
    auto regexNep141Transfer  = llvm::Regex("[0-9]+(ft_transfer(_call)?)[0-9]+");
    auto regexRound           = llvm::Regex("[0-9]+(try_round|round)[0-9]+ ");
    auto regexPartialEq       = llvm::Regex("(core..cmp..PartialEq)");
    auto regexPromiseResult   = llvm::Regex("near_sdk[0-9]+environment[0-9]+env[0-9]+promise_result[0-9]+");

    class Logger {
      private:
        llvm::raw_ostream *os;

        void printOne(const llvm::DebugLoc first) {
            *os << "\e[34m";
            first.print(*os);
            *os << "\e[33m";
        }
        void printOne(const llvm::DebugLoc *first) { printOne(*first); }
        void printOne(const char *first) { *os << first; }

        template <typename T>
        void printOne(T first) {
            *os << first;
        }
        template <typename T>
        void printOne(T *first) {
            *os << *first;
        }
        template <typename T>
        void printOne(std::vector<T> &first) {
            *os << '[';
            if (!first.empty())
                printOne(first.front());
            for (auto itor = first.begin() + 1; itor != first.end(); itor++) {
                *os << ", ";
                printOne(*itor);
            }
            *os << ']';
        }

        template <typename T, typename... Args>
        void printOne(T first, Args... args) {
            printOne(first);
            printOne(args...);
        }
        void printOne(void) {}

      public:
        Logger(llvm::raw_ostream *os) : os(os) {}
        Logger() : os(&llvm::outs()) {}

        template <typename... Args>
        void Debug(Args... args) {
            *os << "\e[33m[?] ";
            printOne(args...);
            *os << "\e[0m\n";
        }
        template <typename... Args>
        void Info(Args... args) {
            *os << "\e[33m[*] ";
            printOne(args...);
            *os << "\e[0m\n";
        }
        template <typename... Args>
        void Warning(Args... args) {
            *os << "\e[33m[!] ";
            printOne(args...);
            *os << "\e[0m\n";
        }
    };

    /**
     * @brief print whatever to string using `print` interface of `whatever` and return the string
     *
     * @param whatever must have `print` interface
     * @return std::string
     */
    template <typename T>
    std::string printToString(T *whatever) {
        std::string srcMeta;
        llvm::raw_string_ostream os_src(srcMeta);
        whatever->print(os_src);
        return srcMeta;
    }

    template <typename T>
    std::string printToString(T whatever) {
        return printToString(&whatever);
    }

    /**
     * @brief find users only using `llvm::Value::users`
     *
     * @param value whose users will be found
     * @param set where users of `value` will be kept
     * @param restrictCrossFunction whether strict the cross function get-user to only the paired args
     */
    void simpleFindUsers(llvm::Value *value, std::set<llvm::Value *> &set, bool restrictCrossFunction = false) {
        using namespace llvm;

        auto const valueType = value->getType();
        if (!valueType || valueType->isLabelTy())
            return;

        if (set.find(value) != set.end())
            return;

        set.insert(value);

        if (auto CallInst = dyn_cast<CallBase>(value)) {
            for (unsigned i = 0; i < CallInst->arg_size(); i++) {
                if (CallInst->getCalledFunction() && (!restrictCrossFunction || set.count(CallInst->getArgOperand(i))))
                    simpleFindUsers(CallInst->getCalledFunction()->getArg(i), set, restrictCrossFunction);  // cross-function
                if (CallInst->getCalledFunction() && CallInst->getCalledFunction()->getName().contains("core..convert..Into") && CallInst->arg_size() >= 2 && set.count(CallInst->getArgOperand(1))) {
                    simpleFindUsers(CallInst->getArgOperand(0), set, restrictCrossFunction);  // add `xxx.into()` as user of `xxx`
                }
            }
        }
        for (auto *U : value->users()) {
            simpleFindUsers(U, set, restrictCrossFunction);
        }
    }

    /**
     * @brief find users of `value`
     *
     * @param value whose users will be found
     * @param set where users of `value` will be kept
     * @param GEPOffset speicify offset of GetElementPtr instruction, by default is -1, which means unspecified
     * @param depth depcth of recursion
     */
    void findUsers(llvm::Value *value, std::set<llvm::Value *> &set, const int GEPOffset = -1, int depth = __INT32_MAX__) {
        using namespace llvm;
        if (depth <= 0)
            return;
        if (set.count(value) == 1)
            return;

        auto const valueType = value->getType();

        if (!valueType || valueType->isLabelTy())
            return;

        set.insert(value);

        if (auto UnaInst = dyn_cast<UnaryInstruction>(value)) {  // alloca, cast, extract, freeze, load, unaryOp, VAA
            if (GEPOffset != -1) {
                if (auto BCInst = dyn_cast<BitCastInst>(value)) {
                    std::string resType = printToString(BCInst->getDestTy()), srcType = printToString(BCInst->getSrcTy());
                    if (GEPOffset != 0 && (StringRef(resType).contains("solana_program::pubkey::Pubkey") && StringRef(srcType).contains("solana_program::account_info::AccountInfo") ||
                                              (StringRef(resType).contains("anchor_lang::prelude::Pubkey") && StringRef(srcType).contains("anchor_lang::prelude::AccountInfo"))))
                        return;  // BitCast Not allowed for Offset!=0
                }
            }
            for (auto *U : UnaInst->users())
                findUsers(U, set, GEPOffset, depth - 1);
        } else if (auto BinOpInst = dyn_cast<BinaryOperator>(value)) {
            for (auto *U : BinOpInst->users())
                findUsers(U, set, GEPOffset, depth - 1);
        } else if (auto GEPInst = dyn_cast<GetElementPtrInst>(value)) {
            bool goFurther = GEPOffset == -1;

            if (!goFurther) {
                std::string resType = printToString(GEPInst->getResultElementType()), srcType = printToString(GEPInst->getSourceElementType());

                switch (dyn_cast<ConstantInt>(GEPInst->getOperand(GEPInst->getNumOperands() - 1))->getZExtValue()) {
                    // more cases can be set
                    case 3:  // owner field
                        if (GEPOffset == 3 && (StringRef(resType).contains("solana_program::pubkey::Pubkey") && StringRef(srcType).contains("solana_program::account_info::AccountInfo")) ||
                            (StringRef(resType).contains("anchor_lang::prelude::Pubkey") && StringRef(srcType).contains("anchor_lang::prelude::AccountInfo")))
                            goFurther = true;
                        break;

                    default: goFurther = true; break;
                }
            }

            if (goFurther)
                for (auto *U : GEPInst->users())
                    findUsers(U, set, GEPOffset, depth - 1);
        } else if (auto StInst = dyn_cast<StoreInst>(value)) {
            findUsers(StInst->getPointerOperand(), set, GEPOffset, depth - 1);
        } else if (auto CallInst = dyn_cast<CallBase>(value)) {
            if (CallInst->getCalledFunction() && CallInst->getCalledFunction()->getName().startswith("llvm.memcpy") && CallInst->getArgOperand(0)) {
                for (auto *U : CallInst->getArgOperand(0)->users())  // memcpy target
                    findUsers(U, set, GEPOffset, depth - 1);
                if (auto CstInst = dyn_cast<CastInst>(CallInst->getArgOperand(0))) {  // memcpy target pointer
                    if (CstInst->getOperand(0)) {
                        for (auto *U : CstInst->getOperand(0)->users())
                            findUsers(U, set, GEPOffset, depth - 1);
                    }
                }
            } else if (CallInst->getCalledFunction() && CallInst->getCalledFunction()->getName().contains("core..cmp")) {
                // ends here, skip compare function
            } else if (GEPOffset != -1 && CallInst->getCalledFunction() && Regex("solana_program[0-9]+program_pack[0-9]+Pack[0-9]+unpack").match(CallInst->getCalledFunction()->getName())) {
                // ends here, skip unpack result for owner-check
            } else if (CallInst->getCalledFunction() && CallInst->getCalledFunction()->getName().contains("core..convert..Into") && CallInst->arg_size() >= 2 &&
                       set.count(CallInst->getArgOperand(1))) {
                findUsers(CallInst->getArgOperand(0), set, GEPOffset, depth - 1);  // add `xxx.into()` as user of `xxx`
            } else {
                // for (unsigned i = 0; i < CallInst->getCalledFunction()->arg_size(); i++)
                //     findUsers(CallInst->getCalledFunction()->getArg(i), set, GEPOffset, depth - 1);  // cross function
                for (unsigned i = 0; i < CallInst->arg_size(); i++)
                    findUsers(CallInst->getArgOperand(i), set, GEPOffset, depth - 1);  // function args
                for (auto *U : CallInst->users())
                    findUsers(U, set, GEPOffset, depth - 1);
            }
        } else if (auto Inst = dyn_cast<Instruction>(value)) {
            // ignore other instructions
        } else {
            for (auto *U : value->users()) {
                if (set.find(U) != set.end())
                    continue;
                findUsers(U, set);
            }
        }
    }

    bool isInstCallFunc(llvm::Instruction *I, llvm::Regex const &regex) {
        if (llvm::CallBase *callInst = llvm::dyn_cast<llvm::CallBase>(I)) {
            if (callInst->getCalledFunction()) {
                llvm::StringRef calleeName = callInst->getCalledFunction()->getName();
                if (regex.match(calleeName)) {
                    return true;
                }
                if (debug_print_derive_pack && calleeName == "llvm.dbg.declare" && callInst->getArgOperand(0)) {
                    std::string argMetadata = printToString(llvm::dyn_cast<llvm::MetadataAsValue>(callInst->getArgOperand(0))->getMetadata());
                    if (regex.match(llvm::StringRef(argMetadata)))
                        return true;
                }
            }
        }
        return false;
    }

    namespace {  // Hide this func
        bool _isInstCallFunc_Rec(llvm::CallGraphNode *node, llvm::Regex const &regex, std::vector<llvm::CallGraphNode *> &callStack) {
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
                if (_isInstCallFunc_Rec(callee.second, regex, callStack))
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
                return _isInstCallFunc_Rec(CG[callInst->getCalledFunction()], regex, callStack);
            }
        }
        return false;
    }

    enum Mode { Read = 0b10, Write = 0b01, RW = 0b11, Unknown = 0b00 };

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
            if (dyn_cast<LoadInst>(user)) {
                // outs() << "load\n";
                return Mode::Read;
            } else if (dyn_cast<StoreInst>(user)) {
                // outs() << "store\n";
                return Mode::Write;
            } else if (auto mcpyInst = dyn_cast<MemCpyInst>(user)) {
                // outs() << "mcpyInst: " << *mcpyInst << "\n";
                // outs() << "prevUser:                 " << *prevUser << "\n";
                // outs() << "mcpyInst->getRawDest():   " << *mcpyInst->getRawDest() << "\n";
                // outs() << "mcpyInst->getRawSource(): " << *mcpyInst->getRawSource() << "\n";
                if (mcpyInst->getRawDest() == prevUser)
                    return Mode::Write;
                else if (mcpyInst->getRawSource() == prevUser)
                    return Mode::Read;
                else
                    return Mode::Unknown;
            } else if (auto callInst = dyn_cast<CallBase>(user)) {
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

                        // outs() << "argsop: ";
                        int user_no = -1;
                        for (auto i = 0; i < callInst->arg_size(); i++) {
                            if (callInst->getArgOperand(i) == prevUser)
                                user_no = i;
                            // outs() << *callInst->getArgOperand(i) << " | ";
                        }
                        // outs() << "\n";

                        // outs() << "user_no: " << user_no << "\n";
                        if (user_no == -1) {  // user not found in args
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
                        user     = callInst->getCalledFunction()->getArg(user_no);
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
                    for (auto i : user->users()) {
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

    /**
     * @brief use GetElementPtrInst and CastInst to find all dereferences to struct pointers
     *
     * @param I pointer to instruction
     * @param vars target list
     * @return std::pair<std::string, unsigned> pair of struct name and member offset; <"", 0> will be returned if not found
     */
    std::pair<std::pair<std::string, unsigned>, Mode> usingStructMember(llvm::Instruction *I, std::set<std::pair<std::string, unsigned>> &vars) {
        using namespace llvm;

        Value *castValue = nullptr;               // pointer to llvm::Value of member that has been cast from struct pointer, also serves as a flag
        std::pair<std::string, unsigned> target;  // pair of struct name and member offset

        if (auto GEPInst = dyn_cast<GetElementPtrInst>(I)) {  // for offset > 0
            for (auto i : vars) {
                std::string srcMeta = printToString(GEPInst->getSourceElementType());
                if (srcMeta.find('=') == std::string::npos)
                    continue;
                std::string structName = srcMeta.substr(1, srcMeta.find('=') - 2);
                structName.erase(std::remove(structName.begin(), structName.end(), '"'), structName.end());

                /* Control exact match */
                // if (structName.find(i.first) != std::string::npos && dyn_cast<ConstantInt>(GEPInst->getOperand(GEPInst->getNumOperands() - 1))->equalsInt(i.second)) {
                if (structName == i.first && dyn_cast<ConstantInt>(GEPInst->getOperand(GEPInst->getNumOperands() - 1))->equalsInt(i.second)) {
                    castValue = dyn_cast<Value>(I);
                    target    = i;
                    break;
                }
            }
        } else if (auto CI = dyn_cast<CastInst>(I)) {  // for offset = 0
            for (auto i : vars) {
                if (i.second != 0)  // skip offset > 0
                    continue;

                std::string srcMeta = printToString(CI->getSrcTy());
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
            int mode        = getMode(user, prevUser);
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

    /**
     * @brief use GetElementPtrInst and CastInst to find all dereferences to struct pointers
     *
     * @param I pointer to instruction
     * @param vars target list
     * @return std::pair<std::string, unsigned> pair of struct name and member offset; <"", Unkown> will be returned if not found
     */
    std::pair<std::string, Mode> usingStruct(llvm::Instruction *I, std::set<std::string> &structs) {
        using namespace llvm;

        Value *castValue = nullptr;  // pointer to llvm::Value of member that has been cast from struct pointer
                                     // also serves as a flag
        std::string target;          // pair of struct name and member offset

        if (auto GEPInst = dyn_cast<GetElementPtrInst>(I)) {  // for offset > 0
            std::string srcMeta = printToString(GEPInst->getSourceElementType());
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
        } else if (auto CI = dyn_cast<CastInst>(I)) {  // for offset = 0
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
            int mode        = getMode(user, prevUser);
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
