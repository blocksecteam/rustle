#!/usr/bin/env python3
import sys

from core import *

PROJ_PATH = "/root/tmp/ref-contracts/ref-exchange"

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: utils/findPub.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: utils/findPub.py [path to project]")
    sys.exit()

for path in getFiles(PROJ_PATH):
    # print('[*] Checking ' + path)
    results = findPub(path)
    for var_name in results.keys():
        print("[*] func: {}".format(var_name))
        func_def = results[var_name]
        print("{}:{}:{})".format(path, func_def[0], func_def[1]))
