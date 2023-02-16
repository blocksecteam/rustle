#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ["NEAR_SRC_DIR"] if os.environ.get("NEAR_SRC_DIR") != None else ""

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/public-interface.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/public-interface.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"] if os.environ.get("TMP_DIR") != None else ".tmp"

os.makedirs(TMP_PATH, exist_ok=True)

with open(TMP_PATH + "/.public-interface.tmp", "w") as out_file:
    for path in getFiles(PROJ_PATH):
        results = findFunc(path)
        for func in results:
            func_name = func["name"]

            if func["visibility"] == "public(near_bindgen)":
                print("[*] public interface " + func["struct"] + "::" + func_name)
                out_file.write(func["struct"] + "::" + func["struct_trait"] + "::" + func_name + "@" + path + "\n")
    out_file.close()
