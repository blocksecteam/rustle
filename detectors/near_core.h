#pragma once

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <utility>
#include <vector>

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"

namespace Rustle {
    // tool config
    std::string const ext_call_trait_file = std::string(getenv("TMP_DIR")) + std::string("/.ext-call-trait.tmp");
    std::string const callback_file       = std::string(getenv("TMP_DIR")) + std::string("/.callback.tmp");
    int const MIN_INST_NUM_FOR_LOOP       = 100;

    bool const debug_check_all_func = true;

    bool const debug_print_function = false;
    bool const debug_print_tmp      = false;
    bool const debug_print_notfound = debug_print_function && false;

    bool const debug_print_derive_pack = false;  // show pack/unpack from `#[derive(Serialize, Deserialize)]`

    // built-in regex
    auto const regexForLibFunc = llvm::Regex("(^/cargo)|(^/rustc)"
                                             "|(_ZN\\d+(core|std|alloc|num_traits|solana_program|byteorder|hex|bytemuck|borsh|enumflags2|safe_transmute|thiserror|byteorder)([0-9]+|\\.\\.)[a-zA-Z]+)"
                                             "|(serde\\.\\.de\\.\\.Deserialize)");
    auto const regexForLibLoc  = llvm::Regex("(^/rustc)|(^/cargo)|(^/root/.cargo)|(^/home/.+/.cargo)|(^$)");

    auto const regexExtCall         = llvm::Regex("(.+near_sdk[0-9]+promise[0-9]+Promise[0-9]+function_call(_weight)?[0-9]+)");
    auto const regexPromiseTransfer = llvm::Regex("near_sdk[0-9]+promise[0-9]+Promise[0-9]+transfer[0-9]+");
    auto const regexNep141Transfer  = llvm::Regex("[0-9]+(ft_transfer(_call)?)[0-9]+");
    auto const regexRound           = llvm::Regex("[0-9]+std[0-9]+.+[0-9]+(try_round|round)[0-9]+");
    auto const regexPartialEq       = llvm::Regex("(core..cmp..PartialEq)");
    auto const regexPartialOrd      = llvm::Regex("(core[0-9]+cmp[0-9]+PartialOrd)");
    auto const regexPromiseResult =
        llvm::Regex("(near_sdk[0-9]+environment[0-9]+env[0-9]+(promise_result|promise_results_count)[0-9]+)|(near_sdk[0-9]+utils[0-9]+(is_promise_success|promise_result_as_success)[0-9]+)");

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
        template <typename... Args>
        void Error(Args... args) {
            *os << "\e[33m[ERROR] ";
            printOne(args...);
            printOne("\n        Exiting...");
            *os << "\e[0m\n";
            std::exit(1);
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
        llvm::raw_string_ostream osSrc(srcMeta);
        whatever->print(osSrc);
        return srcMeta;
    }

    /**
     * @brief convert whatever to a pointer and print its string
     *
     * @param whatever
     * @return std::string
     */
    template <typename T>
    std::string printToString(T whatever) {
        return printToString(&whatever);
    }

    /**
     * @brief find users only using `llvm::Value::users`
     *
     * @param value whose users will be found
     * @param set where users of `value` will be kept
     * @param restrictCrossFunction whether strict the cross function get-user to only the paired args, activated only when `disableCrossFunction == false`
     * @param disableCrossFunction whether allow cross function
     */
    void simpleFindUsers(llvm::Value *value, std::set<llvm::Value *> &set, bool restrictCrossFunction = false, bool disableCrossFunction = false);

    /**
     * @brief find users of `value`
     *
     * @param value whose users will be found
     * @param set where users of `value` will be kept
     * @param GEPOffset specify offset of GetElementPtr instruction, by default is -1, which means unspecified
     * @param depth depth of recursion
     */
    void findUsers(llvm::Value *value, std::set<llvm::Value *> &set, const int GEPOffset = -1, int depth = __INT32_MAX__);

    /**
     * @brief find if a function is privileged, recursion is allowed
     *
     * @param F target function
     * @param depth depth of recursion
     * @return true
     * @return false
     */
    bool isFuncPrivileged(llvm::Function *F, int const depth = 2);

    /**
     * @brief
     *
     * @param I
     * @param regex
     * @return true
     * @return false
     */
    bool isInstCallFunc(llvm::Instruction *I, llvm::Regex const &regex);

    /**
     * @brief recursive version of isInstCallFunc
     *
     * @param I
     * @param CG
     * @param regex
     * @return true
     * @return false
     */
    bool isInstCallFuncRec(llvm::Instruction *I, llvm::CallGraph &CG, llvm::Regex const &regex);

    /**
     * @brief check if F calls function with name matching regex recursively
     *
     * @param F
     * @param CG
     * @param regex
     * @return true
     * @return false
     */
    bool isFuncCallFuncRec(llvm::Function *F, llvm::CallGraph &CG, llvm::Regex const &regex);

    /**
     * @brief find all caller of function F, recursively
     *
     * @param F
     * @param set result set, passed by reference
     * @param depth depth of recursion
     */
    void findFunctionCallerRec(llvm::Function *F, std::set<llvm::Function *> &set, int depth = INT32_MAX);

    /**
     * @brief find all caller of function F
     *
     * @param F
     * @param set result set, passed by reference
     */
    inline void findFunctionCaller(llvm::Function *F, std::set<llvm::Function *> &set) {
        findFunctionCallerRec(F, set, 1);
    }

    enum Mode { Read = 0b10, Write = 0b01, RW = 0b11, Unknown = 0b00 };

    /**
     * @brief use GetElementPtrInst and CastInst to find all dereferences to struct pointers
     *
     * @param I pointer to instruction
     * @param vars target list
     * @return std::pair<std::string, unsigned> pair of struct name and member offset; <"", 0> will be returned if not found
     */
    std::pair<std::pair<std::string, unsigned>, Mode> usingStructMember(llvm::Instruction *I, std::set<std::pair<std::string, unsigned>> &vars);

    /**
     * @brief
     *
     * @param is
     * @param vars
     */
    void readStructMember(std::ifstream &is, std::set<std::pair<std::string, unsigned>> &vars);

    /**
     * @brief use GetElementPtrInst and CastInst to find all dereferences to struct pointers
     *
     * @param I pointer to instruction
     * @param vars target list
     * @return std::pair<std::string, unsigned> pair of struct name and member offset; <"", Unkown> will be returned if not found
     */
    std::pair<std::string, Mode> usingStruct(llvm::Instruction *I, std::set<std::string> &structs);

    /**
     * @brief
     *
     * @param is
     * @param structs
     */
    void readStructLog(std::ifstream &is, std::set<std::string> &structs);

}  // namespace Rustle
