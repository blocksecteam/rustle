#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ["NEAR_SRC_DIR"]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/unused-ret.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/unused-ret.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]
os.makedirs(TMP_PATH, exist_ok=True)

all_funcs = []
for path in getFiles(PROJ_PATH):
    all_funcs += findFunc(path)

all_calls = dict()  # {filename_line, {callee}}
os.system("mv {0} {0}.org; rustfilt -i {0}.org -o {0}; rm {0}.org".format(TMP_PATH + "/.all-call.tmp"))
with open(TMP_PATH + "/.all-call.tmp", "r") as file:
    for line in file:
        callee_llvm, file, line_no = line.strip().split("@")
        if file + "@" + line_no in all_calls.keys():
            all_calls[file + "@" + line_no].append(callee_llvm)
        else:
            all_calls[file + "@" + line_no] = [callee_llvm]


with open(TMP_PATH + "/.unused-ret.tmp", "w") as out_file:
    for path in getFiles(PROJ_PATH):
        impls = findImpl(path)
        with open(path, "r") as file:
            matches_void_call = re.compile(r"(;|\{)\s+((?P<struct_name>\w+)\.)*(?P<name>\w+)\([^\)]+\)\??;", re.MULTILINE | re.DOTALL)
            string = re.sub(r"//[^\n]+\n", "\n", file.read())
            for match in matches_void_call.finditer(string):
                line_no = string[0 : match.end()].count("\n") + 1
                curStruct = lineToStruct(line_no, impls)
                if curStruct["struct"] == None:
                    curStruct["struct"] = ""
                if curStruct["trait"] == None:
                    curStruct["trait"] = ""

                curFunc = line2funcName(line_no, findFunc(path))
                callee_name = match.groupdict()["name"].strip()

                target_func = None
                if match.groupdict()["struct_name"] != None:
                    for loc in all_calls.keys():
                        if (path + "@" + str(line_no)).endswith(loc):
                            for callee_llvm in all_calls[loc]:
                                if callee_name in callee_llvm:
                                    for func in all_funcs:
                                        if structFuncNameMatch(callee_llvm, func["struct"], func["struct_trait"], func["name"], path):
                                            target_func = func
                                            break
                                if target_func != None:
                                    break
                        if target_func != None:
                            break
                else:
                    for i in all_funcs:
                        if i["name"] == callee_name:
                            target_func = i
                            break

                if target_func != None:
                    if target_func["return"] != "" and not re.match(r"Result<\(\)(,\s+(.+))?>", target_func["return"]):
                        print("[!] Function `{}` with return type `{}` is called without using return value in `{}:{}`.".format(callee_name, target_func["return"], path, line_no))
                        out_file.write(curStruct["struct"] + "::" + curStruct["trait"] + "::" + curFunc + "@" + str(line_no) + "@" + callee_name + "\n")
            file.close()
    out_file.close()
