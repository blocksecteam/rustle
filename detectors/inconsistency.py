#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re
from difflib import SequenceMatcher
import json

PROJ_PATH = os.environ["NEAR_SRC_DIR"]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/inconsistency.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/inconsistency.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]
os.makedirs(TMP_PATH, exist_ok=True)

all_vars = dict()
for path in getFiles(PROJ_PATH):
    all_vars = {**all_vars, **findVar(path)}
# print(all_vars)

global_vars = dict()
for path in getFiles(PROJ_PATH):
    global_vars = {**global_vars, **findGlobalVar(path)}
# print(global_vars.keys())


def isSimilar(a, b) -> bool:
    # if (a in b and 2 * len(a) > len(b)) or (b in a and 2 * len(b) > len(a)):
    #     return True
    if SequenceMatcher(None, a, b).ratio() > 0.85:
        return True
    return False


fuzz_set = dict()
for i in global_vars.keys():
    fuzz_set[i] = []
    for j in global_vars.keys():
        if i == j:
            continue
        if isSimilar(i, j):
            fuzz_set[i].append(j)
    if len(fuzz_set[i]) == 0:
        fuzz_set.pop(i)
print(json.dumps(fuzz_set, sort_keys=True, indent=4))

with open(TMP_PATH + "/.inconsistency.tmp", "w") as file:
    json.dump(fuzz_set, file, sort_keys=True, indent=4)
