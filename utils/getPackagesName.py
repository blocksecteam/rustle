#!/usr/bin/env python3
import os
from typing import Optional

import toml
from core import *

TMP_PATH = os.environ["TMP_DIR"]


def getName(path) -> Optional[str]:
    with open(path, "r") as cargo_file:
        parsed_cargo = toml.loads(cargo_file.read())
        package = parsed_cargo.get("package")
        if package != None and package["name"] != None:
            return package["name"].replace("-", "_")
    return None


with open(TMP_PATH + "/.packages-name.tmp", "w") as log_file:
    for path in os.environ["TG_MANIFESTS"].split(" "):
        name = getName(path)
        if name != None:
            log_file.write(name + "\n")
    log_file.close()
