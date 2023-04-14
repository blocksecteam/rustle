#!/usr/bin/env python3
import os
import sys

from core import *

PROJ_PATH = "/root/tmp/ref-contracts/ref-exchange"

if len(sys.argv) == 1:
    PROJ_PATH = os.environ["NEAR_SRC_DIR"]
elif len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: utils/findStruct.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: utils/findStruct.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]

os.makedirs(TMP_PATH, exist_ok=True)
file_w_member = open(TMP_PATH + "/.struct-members.tmp", "w")
file_wo_member = open(TMP_PATH + "/.structs.tmp", "w")

results_list = []

for path in getFiles(PROJ_PATH):
    # print('[*] Checking ' + path)
    results = findStruct(path)
    if len(results) == 0:
        continue
    results_list.append(results)

file_w_member.write("{}\n".format(sum(len(i) for i in results_list)))

for results_single_file in results_list:
    for struct in results_single_file:
        struct_name = struct["name"]
        # print('\n[*] {}'.format(struct_name))
        file_wo_member.write("{}\n".format(struct_name))
        file_w_member.write("{}@{}@{}\n".format(struct_name, len(struct["members"]), struct["file"]))
        member_list = struct["members"]
        for i in member_list:
            # print(" +- {}: {}".format(i['name'], i['type']))
            file_w_member.write("{}\n{}\n".format(i["name"], i["type"]))
