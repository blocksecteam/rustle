#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ["NEAR_SRC_DIR"]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/lock-callback.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/lock-callback.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]
os.makedirs(TMP_PATH, exist_ok=True)

callback_func_set = set()
os.system("mv {0} {0}.org; rustfilt -i {0}.org -o {0}; rm {0}.org".format(TMP_PATH + "/.callback.tmp"))
with open(TMP_PATH + "/.callback.tmp", "r") as f:
    for line in f:
        func, file = line.strip().split("@")
        callback_func_set.add((func, file))

with open(TMP_PATH + "/.lock-callback.tmp", "w") as out_file:
    for path in getFiles(PROJ_PATH):
        results = findFunc(path)
        for func in results:
            func_name = func["name"]
            func_type = "Function"
            for cb_name in callback_func_set:
                if structFuncNameMatch(cb_name[0], func["struct"], func["struct_trait"], func_name, path):
                    if " (callback)" not in func_type:
                        func_type += " (callback)"
                    break

            if " (callback)" in func_type:
                if re.match("assert!|require!", func["modifier"]):
                    print("[!] assertion in callback function " + func_name + " may lock contract.")
                    out_file.write(func["struct"] + "::" + func["struct_trait"] + "::" + func_name + "@" + path + "\n")
    out_file.close()
