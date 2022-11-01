#!/usr/bin/env python3
import sys
import os
sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ['NEAR_SRC_DIR']

if len(sys.argv) == 2:
    if sys.argv[1] == '-h' or sys.argv[1] == '--help':
        print('Usage: detectors/tautology.py [path to project]')
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print('Usage: detectors/tautology.py [path to project]')
    sys.exit()

for path in getFiles(PROJ_PATH):
    # print('[*] Checking ' + path)
    results = regexInFile(path, r'(true\s*\|\|)|(\|\|\s*true)|(false\s*&&)|(&&\s*false)')
    if(len(results) != 0):
        for result in results:
            print("{}:{}:{}".format(path, result[0], result[1]))
