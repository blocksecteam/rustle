#!/usr/bin/env python3
import pytablewriter
import sys
from tqdm import tqdm
from core import *
import re
import os
import json


import toml
TMP_PATH = os.environ['TMP_DIR']

def getName(path) -> str:
    with open(path, 'r') as cargo_file:
        parsed_cargo = toml.loads(cargo_file.read())
        package = parsed_cargo.get('package')
        if package != None and package['name'] != None:
            return(package['name'].replace('-', '_'))
    return None

with open(TMP_PATH + '/.packages-name.tmp', 'w') as log_file:
    for path in os.environ['TG_MANIFESTS'].split(' '):
        log_file.write(getName(path) + '\n')
    log_file.close()
