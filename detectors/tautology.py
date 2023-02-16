#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ["NEAR_SRC_DIR"]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/tautology.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/tautology.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]
os.makedirs(TMP_PATH, exist_ok=True)

with open(TMP_PATH + "/.tautology.tmp", "w") as out_file:
    for path in getFiles(PROJ_PATH):
        # print('[*] Checking ' + path)
        funcs = findFunc(path)
        results = regexInFile(path, r"(true\s*\|\|)|(\|\|\s*true)|(false\s*&&)|(&&\s*false)")
        if len(results) != 0:
            for line_no, line in results:
                print("{}:{}:{}".format(path, line_no, line))
                func = line2func(line_no, funcs)
                if func != None:
                    out_file.write(func["struct"] + "::" + func["struct_trait"] + "::" + func["name"] + "@" + path + "@" + str(line_no) + "\n")
    out_file.close()
