import glob
import re
import sys
from typing import Optional


def getFiles(path, ignoreTest=False, ignoreMock=False) -> set:
    """Get all rust files in the given path"""
    return (
        set(glob.glob(path + "/**/*.rs", recursive=True))
        - set(glob.glob(path + "/**/node_modules/**/*.rs", recursive=True))
        - set(glob.glob(path + "/**/target/**/*.rs", recursive=True))
        - (set(glob.glob(path + "/**/*test*/**/*.rs", recursive=True)) if ignoreTest else set())
        - (set(glob.glob(path + "/**/*mock*/**/*.rs", recursive=True)) if ignoreMock else set())
    )


def readFile(path) -> str:
    """Read the file at the given path, return

    Returns:
        str: file content string
    """
    with open(path, "r") as f:
        return f.read()


def regexInFile(filename, pattern) -> list:
    """Search for the given string in file and return lines containing that string, along with line numbers
    todo: cross line tautology

    Returns:
        list: list of tuples containing line number and line content
    """
    count = 0
    results = []
    with open(filename, "r") as file:
        for line in file:
            count += 1
            if re.match(r"\s*//.+", line):  # skip comments
                continue
            if len(re.findall(pattern, line)) != 0:
                results.append((count, line.rstrip()))
    return results


def defInFileWithDepth(filename) -> dict:
    """Search for var def, along with line numbers and depth
    todo: {/} can't be in the same line as definition

    Returns:
        dict: dict key is var name, value is tuple of (line number, line content, depth, namespace)
    """
    pattern = r".*((let\s+(mut\s+)?)|const\s+)(?P<var_name>\w+)(\s*:\s*\w+)?(\s*=\s*.+)?;.*"

    line_number = 0
    depth = 0
    namespaces = [1]
    results = dict()
    with open(filename, "r") as file:
        for line in file:
            line_number += 1

            if re.match("\\s*//.+", line):  # skip comments
                continue

            depth += line.count("{") - line.count("}")
            if line.count("{") > line.count("}"):
                for i in range(line.count("{") - line.count("}")):
                    namespaces.append(line_number)
            elif line.count("{") < line.count("}"):
                for i in range(line.count("}") - line.count("{")):
                    namespaces.pop(len(namespaces) - 1)

            matches = re.match(pattern, line)
            if matches != None:
                var_name = matches.groupdict()["var_name"]
                if var_name in results.keys():
                    results[var_name].append(
                        (
                            line_number,
                            line.rstrip(),
                            depth,
                            namespaces[len(namespaces) - 1],
                        )
                    )
                else:
                    results[var_name] = [
                        (
                            line_number,
                            line.rstrip(),
                            depth,
                            namespaces[len(namespaces) - 1],
                        )
                    ]
    return results


def findPub(filename) -> dict:
    pattern = r".*(pub)\s+(fn)\s+(?P<func_name>\w+).*"

    line_number = 0
    results = dict()
    with open(filename, "r") as file:
        for line in file:
            line_number += 1

            if re.match(r"\s*//.+", line):  # skip comments
                continue

            matches = re.match(pattern, line)
            if matches != None:
                func_name = matches.groupdict()["func_name"]
                results[func_name] = (line_number, line.rstrip())
    return results


def line2func(line, results) -> Optional[dict]:
    """Convert line number to function name"""
    closestDist = sys.maxsize
    closestFunc = None
    for func in results:
        if line > func["line_number"]:
            dist = line - func["line_number"]
            if dist < closestDist:
                closestDist = dist
                closestFunc = func
    return closestFunc


def line2funcName(line, results) -> str:
    """Convert line number to function name"""
    closestDist = sys.maxsize
    closestFuncName = ""
    for func in results:
        if line > func["line_number"]:
            dist = line - func["line_number"]
            if dist < closestDist:
                closestDist = dist
                closestFuncName = func["name"]
    return closestFuncName


def _name2index(name, results) -> int:
    """Convert var name to index"""
    for i in range(len(results)):
        if results[i]["name"] == name:
            return i
    return -1


def lineToStruct(line, impls) -> dict:
    closestDist = sys.maxsize
    closestStruct = {"struct": "", "trait": ""}
    for impl in impls:
        if line >= impl["line_number"]:
            dist = line - impl["line_number"]
            if dist < closestDist:
                closestDist = dist
                closestStruct = impl
    return closestStruct


def findImpl(filename, includeMod=False) -> list:
    results = []
    with open(filename, "r") as file:
        string = re.sub(r" +//[^\n]+\n", "\n", file.read())
        # while '\n\n\n' in string:
        #     string = re.sub(r'\n\s+\n', r'\n\n', string)
        # string = re.sub(r'#\[cfg\(test\)\]\n+mod\s+\w+\s*\{(.|\s)+\n\}', '', string)

        pattern = (
            r"(impl" + (r"|mod" if includeMod else r"") + r")\s+((?P<trait>[^\s]+)\s+for\s+)?(?P<struct>[^\s]+)\s*\{"
        )
        # for func in re.findall(pattern, re.sub('/\\*.+\\*/', '', re.sub('//.+\n', '\n', file.read()))):  # remove comment
        # for func in re.findall(pattern, re.sub('//.+\n', '\n', file.read())):  # remove comment

        matches = re.compile(pattern, re.MULTILINE | re.DOTALL)
        for match in matches.finditer(string):
            results.append(
                {
                    "struct": match.groupdict()["struct"] if match.groupdict()["struct"] else "",
                    "trait": match.groupdict()["trait"] if match.groupdict()["trait"] else "",
                    "file": filename,
                    "line_number": string[0 : match.start()].count("\n") + 1,
                }
            )
        return results


def findFunc(filename, ignoreTest=False) -> list:
    """Search for func, return name and some attributes

    Returns:
        dict: dict key is var name, dict value is a nested dict of attributes
    """

    impls = findImpl(filename, includeMod=True)
    results = []

    # function definition
    with open(filename, "r") as file:
        string = re.sub(r" +//[^\n]+\n", "\n", file.read()).replace("#[allow(unused)]", "")
        # while '\n\n\n' in string:
        #     string = re.sub(r'\n\s+\n', r'\n\n', string)
        string = re.sub(r"#\[cfg\(test\)\]\n+mod\s+\w+\s*\{(.|\s)+\n\}", "", string)

        pattern = r'(?P<macro>(#\[[^\]]+\]\s*)+\n)?(?P<indent> *)(?P<vis>pub\s*)?(?P<hasCrate>\(crate\)\s*)?(extern\s+"C"\s*)?(async\s*)?fn\s+(?P<name>\w+)\s*(?P<trait><[^>]+>)?\s*\((?P<args>[^\)]*)\)\s*(->\s*(?P<return>[^\{\;]+))?\s*(where[^\{]*)?\s*\{'

        matches = re.compile(pattern, re.MULTILINE | re.DOTALL)
        for match in matches.finditer(string):
            if ignoreTest and match.groupdict()["macro"] and "#[test]" in match.groupdict()["macro"]:
                continue
            results.append(
                {
                    "name": match.groupdict()["name"],
                    # only '\n' means the func has no indent
                    "struct": lineToStruct(string[0 : match.start()].count("\n") + 2, impls)["struct"]
                    if " " in match.groupdict()["indent"]
                    else "",
                    "struct_trait": lineToStruct(string[0 : match.start()].count("\n") + 2, impls)["trait"]
                    if " " in match.groupdict()["indent"]
                    else "",
                    "line_number": string[0 : match.start()].count("\n") + 2,  # not accurate
                    "modifier": "",
                    "macro": ""
                    if match.groupdict()["macro"] == None
                    else match.groupdict()["macro"].replace(" ", "").strip("\n").replace("\n", "; "),
                    "visibility": "internal"
                    if (
                        match.groupdict()["macro"]
                        and "#[private]" in match.groupdict()["macro"]
                        or match.groupdict()["hasCrate"]
                    )
                    else (
                        "public"
                        if match.groupdict()["vis"] and match.groupdict()["vis"].startswith("pub")
                        else "private"
                    ),
                    "args": re.sub("//.+\n", "", match.groupdict()["args"])
                    .replace(" ", "")
                    .replace("\n", "")
                    .replace(":", ": ")
                    .replace(",", ", "),
                    "return": match.groupdict()["return"].strip() if match.groupdict()["return"] else "",
                    # 'callback': '',
                }
            )
    if len(results) == 0:
        return []

    line_number = 0
    with open(filename, "r") as file:
        func_name = ""  # since function name is re-def every for loop, use this to keep the one before this line
        last_impl_bindgen = False
        prev_line = ""

        for line in file:
            line_number += 1

            if re.match(r"\s*//.+", line):  # skip comments
                continue

            """
            modify line number & visibility
            """
            pattern = r".+fn\s+(?P<func_name>\w+).+"
            matches = re.match(pattern, line)
            if matches != None:
                func_name = matches.groupdict()["func_name"]
                for i in range(len(results)):
                    if results[i]["name"] == func_name and abs(results[i]["line_number"] - line_number) <= 4:
                        results[i].update(line_number=line_number)  # correct line number
                        # if last_impl_bindgen == False and re.match('\\s+.*', line):  # dont change global function
                        #     results[i].update(visibility='private')
                        if last_impl_bindgen == True:
                            # results[i].update(visibility=results[i]['visibility'] + '(near_bindgen)')
                            results[i]["visibility"] += "(near_bindgen)"
                        break
            """
            modify visibility using #[near_bindgen]
            """
            if re.match(r"impl\s*\w+\s*{\s*", line):
                last_impl_bindgen = "#[near_bindgen]" in prev_line
            if re.match(r"}\s*", line):
                last_impl_bindgen = False

            """
            save state
            """
            prev_line = line

    """
    add modifier
    """
    with open(filename, "r") as file:
        string = re.sub("//.+\n", "\n", file.read())
        matches = re.compile(r"(assert|require)!\([^;]+\);", re.MULTILINE | re.DOTALL)
        for match in matches.finditer(string):
            start, end = string[0 : match.start()].count("\n"), string[0 : match.end()].count("\n")
            func_name = line2funcName(start, results)
            results[_name2index(func_name, results)].update(
                modifier=results[_name2index(func_name, results)]["modifier"]
                + re.sub("\n +", " ", match.group().strip()).replace('"', "").replace("'", "")
                + " "
            )

    # '''
    # add callback
    # '''
    # with open(filename, 'r') as file:
    #     string = re.sub('//.+\n', '\n', file.read())
    #     # matches = re.compile('\\.then\\s*\\(\\s*(\\w+::)?(?P<callback_func>\\w+)\\s*\\(', re.MULTILINE | re.DOTALL)
    #     matches = re.compile(r'\.then\s*\([^\.]+\.(?P<callback_func>(\w+::)?\w+)\s*\(', re.MULTILINE | re.DOTALL)
    #     for match in matches.finditer(string):
    #         start, end = string[0:match.start()].count("\n"), string[0:match.end()].count("\n")
    #         func_name = line2funcName(start, results)
    #         results[_name2index(func_name, results)].update(callback=match.groupdict()['callback_func'])

    return results


def findVar(filename) -> dict:
    results = dict()
    with open(filename, "r") as file:
        string = re.sub("//.+\n", "\n", file.read())
        matches = re.compile(
            r"let\s+(?P<name>\w+)\s*(:\s*(?P<type>[^=]+))?\s*(=\s*(?P<init_val>[^;]+))?;",
            re.MULTILINE | re.DOTALL,
        )
        for match in matches.finditer(string):
            line_no = string[0 : match.start()].count("\n")
            results[match.groupdict()["name"]] = {
                "name": match.groupdict()["name"],
                "type": match.groupdict()["type"],
            }
    return results


def findGlobalVar(filename) -> dict:
    results = dict()
    with open(filename, "r") as file:
        pattern = r"\n(pub\s+)?(const\s+)?(static\s+)?(mut\s+)?(?P<name1>\w+)\s*(:\s*(?P<type2>[^=]+))?\s*(=\s*(?P<value3>[^;]+))?;"
        for var in re.findall(pattern, re.sub("//.+\n", "", file.read())):  # remove comment
            if var[0] == "" and var[1] == "" and var[2] == "" and var[3] == "":
                continue
            # print(var)
            name = var[4]
            results[name] = {
                "value": var[-1].strip(),
                "type": var[-3].strip(),
                "visibility": "public" if "pub" in var[0] else "private",
            }
    return results


def findStruct(filename, only_near_bindgen=False) -> list:
    """Search for struct(with #[near_bindgen]), return name and member list"""
    line_number = 0
    results = []
    with open(filename, "r") as file:
        # print(re.sub('//.+\n', '', file.read()))
        struct_pat = (
            r"(#\[near_bindgen\]\s*)" if only_near_bindgen else ""
        ) + r"(#\[[^\]]+\]\s*)*pub\s+struct\s+(?P<name>\w+)\s*(?P<trait><[^>]+>)?\s*\{(?P<members>[^\}]+)\}"
        member_pat = r"(?P<name>\w+)\s*:\s*(?P<type>.+)\s*,"
        st_matches = re.compile(struct_pat, re.MULTILINE | re.DOTALL)
        for struct in st_matches.finditer(
            re.sub(r"/\*.+\*/", "", re.sub("//.+\n", "\n", file.read()))
        ):  # remove comment
            members = []
            for member in re.findall(member_pat, struct.groupdict()["members"]):
                members.append({"name": member[0], "type": member[1]})
            results.append(
                {
                    "name": struct.groupdict()["name"],
                    "members": members,
                    "file": filename,
                }
            )
        # print(results)
    return results


def findCallbackFunc(filename) -> set:
    results = set()
    with open(filename, "r") as file:
        string = re.sub("//.+\n", "\n", file.read())
        matches = re.compile(
            r"\.then\s*\(\s*(\w+::)?(?P<callback_func>\w+)\s*\(",
            re.MULTILINE | re.DOTALL,
        )
        for match in matches.finditer(string):
            results.add(match.groupdict()["callback_func"])
    return results


def structFuncNameMatch(
    func_string,
    struct,
    trait,
    func,
    path,
    detector_path=None,
    rustle_format=False,
) -> bool:
    """match func_string with provided args

    Args:
        func_string (string): string input
        struct (string): struct from findFunc()
        trait (string): struct_trait from findFunc()
        func (string): name from from findFunc()
        path (string): path from getFiles()
        detector_path (string, optional): path of file output by detector
        rustle_format (bool, optional): Whether the `func_string` is generate by Rustle rather than LLVM. Defaults to False.

    Returns:
        bool: match or not
    """
    if rustle_format:
        if struct == None:
            struct = ""
        if trait == None:
            trait = ""
        return struct + "::" + trait + "::" + func == func_string
    else:
        if detector_path != None and not path.endswith(detector_path):
            return False
        if struct == None or struct == "":
            return func_string.endswith(path.split("/")[-1].split(".")[0] + "::" + func)
        if trait == None or trait == "":
            return func_string.endswith(struct + "::" + func) or func_string.endswith(struct + ">::" + func)
        return func_string.endswith("::" + func) and (
            ("::" + struct + " as" in func_string)
            and ("::" + trait + ">" in func_string)
            or ("::" + trait + " for" in func_string)
            and ("::" + struct + ">" in func_string)
        )
