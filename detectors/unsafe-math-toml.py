#!/usr/bin/env python3
import sys
import os

sys.path.insert(0, os.getcwd())
from utils.core import *
import re
import glob
import toml

PROJ_PATH = os.environ["NEAR_SRC_DIR"]

if len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: detectors/unsafe-math-toml.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: detectors/unsafe-math-toml.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]
os.makedirs(TMP_PATH, exist_ok=True)


def checkToml(path) -> bool:
    checked = False
    with open(path, "r") as cargo_file:
        parsed_cargo = toml.loads(cargo_file.read())
        profile = parsed_cargo.get("profile")
        if profile != None:
            if profile.get("release"):  # defaults to False
                if profile.get("release").get("overflow-checks") == None or profile.get("release").get("overflow-checks") == False:
                    print("[!] Lack of overflow-checks in release profile at " + path + ".")
                    log_file.write(path + "\n")
                    checked = True
            if profile.get("dev"):  # defaults to True
                if profile.get("dev").get("overflow-checks") != None and profile.get("dev").get("overflow-checks") == False:
                    print("[!] Lack of overflow-checks in dev profile at " + path + ".")
                    log_file.write(path + "\n")
                    checked = True
    return checked


def findCargoToml(path) -> list:
    toml_list = glob.glob(path + "/**/Cargo.toml", recursive=True)
    path = os.path.abspath(path + "/..")
    while os.path.exists(path + "/Cargo.toml"):
        toml_list.append(path + "/Cargo.toml")
        path = os.path.abspath(path + "/..")
    return toml_list


with open(TMP_PATH + "/.unsafe-math-toml.tmp", "w") as log_file:
    for path in findCargoToml(PROJ_PATH):
        checkToml(path)
