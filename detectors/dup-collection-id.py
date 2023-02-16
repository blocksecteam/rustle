#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re

PROJ_PATH = os.environ["NEAR_SRC_DIR"] if os.environ.get("NEAR_SRC_DIR") != None else ""

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/dup-collection-id.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/dup-collection-id.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"] if os.environ.get("TMP_DIR") != None else ".tmp"
os.makedirs(TMP_PATH, exist_ok=True)

id_map = dict()

for path in getFiles(PROJ_PATH):
    funcs = findFunc(path)
    with open(path, "r") as file:
        for match in re.compile(r"(?P<name>\w+):\s*(?P<collection>(LazyOption|LegacyTreeMap|LookupMap|LookupSet|TreeMap|UnorderedMap|UnorderedSet|Vector))" r"::new\((?P<id>(\w|:)+)\),").finditer(
            file.read()
        ):
            groupdict = match.groupdict()
            if groupdict["id"] != None:
                if groupdict["id"] in id_map:
                    id_map[groupdict["id"]].add(groupdict["name"])
                else:
                    id_map[groupdict["id"]] = {groupdict["name"]}
        file.close()


with open(TMP_PATH + "/.dup-collection-id.tmp", "w") as out_file:
    for id, name_list in id_map.items():
        if len(name_list) > 1:
            print("[!] collection id `{}` used in multiple collections: {}".format(id, name_list).replace("'", "`"))
            out_file.write("collection id `{}` used in multiple collections: {}; ".format(id, sorted(list(name_list))).replace("'", "`"))
    out_file.close()
