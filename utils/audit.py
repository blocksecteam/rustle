#!/usr/bin/env python3
import json
import os
import re
import sys

from core import *
from pytablewriter.writer.text._csv import CsvTableWriter
from pytablewriter.writer.text._json import JsonTableWriter
from tqdm import tqdm

CSV_PATH = "./audit-result"
if "CSV_PATH" in os.environ.keys():
    CSV_PATH = os.environ["CSV_PATH"]

PROJ_PATH = os.environ["NEAR_SRC_DIR"]
if len(sys.argv) == 1:
    PROJ_PATH = os.environ["NEAR_SRC_DIR"]
elif len(sys.argv) == 2:
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print("Usage: near/audit.py [path to project]")
        sys.exit()
    else:
        PROJ_PATH = sys.argv[1]
elif len(sys.argv) > 2:
    print("Usage: near/audit.py [path to project]")
    sys.exit()

TMP_PATH = os.environ["TMP_DIR"]

PROJECT_NAME = PROJ_PATH.split("/")[-1]

os.makedirs(CSV_PATH, exist_ok=True)
for i in os.listdir(CSV_PATH):
    # if i.startswith('near_audit-') and i.endswith('.csv'):
    os.remove(CSV_PATH + "/" + i)

promise_results_set = set()  # func, file, line
ext_call_set = set()  # func, file, line
reentrancy_set = set()  # func, file, line
complex_loop_set = set()  # func, file, line
transfer_set = set()  # func, file, line
round_set = set()  # func, file, line
div_before_mul_set = set()  # func, file, line
unsafe_math_set = set()  # func, file, line
upgrade_func_set = set()  # func, file
self_transfer_set = set()  # func, check
timestamp_set = set()  # func, file, line
prepaid_gas_set = set()  # func, check
unhandled_promise_set = set()  # func, file, line
yocto_attach_set = set()  # func, file
incorrect_json_set = set()  # func, file, note
storage_gas_set = set()  # func, check
unregistered_receiver_set = set()  # func, check
unsaved_changes_set = set()  # func, file, line
unimplemented_interface_list = list()  # func
unclaimed_storage_fee_set = set()  # func, check
nft_approval_check_set = set()  # func, check
nft_owner_check_set = set()  # func, check


# deadcode_set = set()
callback_func_set = set()

unused_ret_dict = dict()  # <caller, <line, callee>>
inconsistency_dict = dict()
structMember_dict = dict()

lock_callback_set = set()  # func, file
public_interface_set = set()  # func, file
non_cb_private_set = set()  # func, file
non_pri_callback_set = set()  # func, file

unique_collection_id_log = ""  # log can be printed without processing

for tmpFile in glob.glob(TMP_PATH + "/.*.tmp"):
    os.system("mv {0} {0}.org; rustfilt -i {0}.org -o {0}; rm {0}.org".format(tmpFile))

PRINT_LOG_NOT_FOUND = False

try:
    with open(TMP_PATH + "/.callback.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            callback_func_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.promise-result.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            promise_results_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.ext-call.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            ext_call_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.reentrancy.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            reentrancy_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.complex-loop.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            complex_loop_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.transfer.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            transfer_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.round.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            round_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.div-before-mul.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            div_before_mul_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unsafe-math.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            unsafe_math_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.upgrade-func.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            upgrade_func_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.self-transfer.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            self_transfer_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.timestamp.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            timestamp_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.prepaid-gas.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            prepaid_gas_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unhandled-promise.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            unhandled_promise_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.yocto-attach.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            yocto_attach_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unused-ret.tmp", "r") as f:
        for line in f:
            caller, line, callee = line.strip().split("@")
            if caller not in unused_ret_dict.keys():
                unused_ret_dict[caller] = set()
            unused_ret_dict[caller].add((line, callee))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.inconsistency.tmp", "r") as f:
        inconsistency_dict = json.load(f)
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.lock-callback.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            lock_callback_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.non-callback-private.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            non_cb_private_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.non-private-callback.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            non_pri_callback_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.incorrect-json-type.tmp", "r") as f:
        for line in f:
            func, file, note = line.strip().split("@")
            incorrect_json_set.add((func, file, note))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.public-interface.tmp", "r") as f:
        for line in f:
            func, file = line.strip().split("@")
            public_interface_set.add((func, file))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.dup-collection-id.tmp", "r") as f:
        unique_collection_id_log = f.read()
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.storage-gas.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            storage_gas_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unregistered-receiver.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            unregistered_receiver_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unsaved-changes.tmp", "r") as f:
        for line in f:
            func, file, line = line.strip().split("@")
            unsaved_changes_set.add((func, file, int(line)))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    for filename in sorted(glob.glob(TMP_PATH + "/.nep*-interface.tmp")):
        file_unimplemented_interface_list = []
        with open(filename, "r") as f:
            for line in f:
                file_unimplemented_interface_list.append(line.strip())
        unimplemented_interface_list.extend(sorted(file_unimplemented_interface_list))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.unclaimed-storage-fee.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            unclaimed_storage_fee_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.nft-approval-check.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            nft_approval_check_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.nft-owner-check.tmp", "r") as f:
        for line in f:
            func, check = line.strip().split("@")
            check = check.lower() == "true"
            nft_owner_check_set.add((func, check))
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

try:
    with open(TMP_PATH + "/.struct-members.tmp", "r") as f:
        structNum = int(f.readline())
        for i in range(structNum):
            structName, structMemNum, structFile = f.readline().split("@")
            structMemNum = int(structMemNum)
            structMember_dict[structName] = dict()
            for j in range(structMemNum):
                memName = f.readline().strip()
                memType = f.readline().strip()
                structMember_dict[structName][memName] = memType  # <memberName, memberType>
except Exception as e:
    if PRINT_LOG_NOT_FOUND:
        print("Tmp log not found: ", e)

# print(inconsistency_dict)


# for path in getFiles(PROJ_PATH):
#     callback_func_set.update(findCallbackFunc(path))


summary_writer = CsvTableWriter()
summary_writer.headers = ["file", "name", "high", "medium", "low", "info"]
summary_value_matrix = []
json_summary_writer = JsonTableWriter()
json_summary_writer.headers = ["file", "name", "high", "medium", "low", "info"]
json_value_matrix = []

"""
add per-function note
"""
for path in tqdm(getFiles(PROJ_PATH, ignoreTest=True, ignoreMock=True)):
    if "/src/" not in path:
        continue
    writer = CsvTableWriter()
    writer.headers = [
        "name",
        "struct",
        "description",
        "modifier",
        "macro",
        "visibility",
        "status",
        "type",
        "high",
        "medium",
        "low",
        "info",
    ]
    value_matrix = []
    results = findFunc(path)
    # print(results)
    for func in results:
        func_name = func["name"]
        if "test" in func["macro"]:
            continue

        # func_type = 'Function' + (' (callback)' if func['return'] and 'PromiseResult' in func['return'] else '')
        func_type = "Function"
        note_high = ""
        note_medium = ""
        note_low = ""
        note_info = ""
        for cb_name in callback_func_set:
            if structFuncNameMatch(cb_name[0], func["struct"], func["struct_trait"], func_name, path):
                if " (callback)" not in func_type:
                    func_type += " (callback)"
                break

        # if func['callback'] != '':
        #     note_info += 'callback: <' + func['callback'] + '>; '

        for re_line in reentrancy_set:
            if structFuncNameMatch(re_line[0], func["struct"], func["struct_trait"], func_name, path, re_line[1]):
                note_high += "possible reentrancy; "
                break

        hasPrint = False
        for cl_name in complex_loop_set:
            if structFuncNameMatch(cl_name[0], func["struct"], func["struct_trait"], func_name, path, cl_name[1]):
                note_info += ("" if hasPrint else "loop with complex logic at <") + "L" + str(cl_name[2]) + " "
                hasPrint = True
        if hasPrint:
            note_info = note_info.rstrip() + ">; "

        hasPrint = False
        for tf_name in transfer_set:
            if structFuncNameMatch(tf_name[0], func["struct"], func["struct_trait"], func_name, path, tf_name[1]):
                note_info += ("" if hasPrint else "transfer at <") + "L" + str(tf_name[2]) + " "
                hasPrint = True
        if hasPrint:
            note_info = note_info.rstrip() + ">; "

        hasPrint = False
        for rd_name in round_set:
            if structFuncNameMatch(rd_name[0], func["struct"], func["struct_trait"], func_name, path, rd_name[1]):
                note_medium += ("" if hasPrint else "rounding at <") + "L" + str(rd_name[2]) + " "
                hasPrint = True
        if hasPrint:
            note_medium = note_medium.rstrip() + ">; "

        hasPrint = False
        for dbm_name_line in div_before_mul_set:
            if structFuncNameMatch(
                dbm_name_line[0], func["struct"], func["struct_trait"], func_name, path, dbm_name_line[1]
            ):
                note_medium += ("" if hasPrint else "div-before-mul at <") + "L" + str(dbm_name_line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_medium = note_medium.rstrip() + ">; "

        hasPrint = False
        for sm_name_line in unsafe_math_set:
            if structFuncNameMatch(
                sm_name_line[0], func["struct"], func["struct_trait"], func_name, path, sm_name_line[1]
            ):
                note_high += ("" if hasPrint else "unsafe math at <") + "L" + str(sm_name_line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_high = note_high.rstrip() + ">; "

        hasPrint = False
        for ts_name_line in timestamp_set:
            if structFuncNameMatch(
                ts_name_line[0], func["struct"], func["struct_trait"], func_name, path, ts_name_line[1]
            ):
                note_info += ("" if hasPrint else "timestamp use at <") + "L" + str(ts_name_line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_info = note_info.rstrip() + ">; "

        for uf_name_line in upgrade_func_set:
            if structFuncNameMatch(
                uf_name_line[0], func["struct"], func["struct_trait"], func_name, path, uf_name_line[1]
            ):
                note_info += "upgrade func; "
                break

        for st_name_line in self_transfer_set:
            if st_name_line[1] == False and structFuncNameMatch(
                st_name_line[0], func["struct"], func["struct_trait"], func_name, path
            ):
                note_high += "require self-transfer check; "
                break

        for pg_name_line in prepaid_gas_set:
            if pg_name_line[1] == False and structFuncNameMatch(
                pg_name_line[0], func["struct"], func["struct_trait"], func_name, path
            ):
                note_low += "require prepaid_gas check; "
                break

        hasPrint = False
        for up_line in unhandled_promise_set:
            if structFuncNameMatch(up_line[0], func["struct"], func["struct_trait"], func_name, path, up_line[1]):
                note_high += ("" if hasPrint else "unhandled promise at <") + "L" + str(up_line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_high = note_high.rstrip() + ">; "

        hasPrint = False
        for line in ext_call_set:
            if structFuncNameMatch(line[0], func["struct"], func["struct_trait"], func_name, path, line[1]):
                note_info += ("" if hasPrint else "call external function at <") + "L" + str(line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_info = note_info.rstrip() + ">; "

        hasPrint = False
        for line in promise_results_set:
            if structFuncNameMatch(line[0], func["struct"], func["struct_trait"], func_name, path, line[1]):
                note_info += ("" if hasPrint else "promise_result at <") + "L" + str(line[2]) + " "
                hasPrint = True
        if hasPrint:
            note_info = note_info.rstrip() + ">; "

        for ya_line in yocto_attach_set:
            if structFuncNameMatch(ya_line[0], func["struct"], func["struct_trait"], func_name, path, ya_line[1]):
                note_medium += "require assert_one_yocto check for privilege function; "
                break

        for lc_line in lock_callback_set:
            if structFuncNameMatch(
                lc_line[0], func["struct"], func["struct_trait"], func_name, path, lc_line[1], rustle_format=True
            ):
                note_medium += "assert in callback function may lock contract when failed; "
                break

        for line in non_cb_private_set:
            if structFuncNameMatch(
                line[0], func["struct"], func["struct_trait"], func_name, path, line[1], rustle_format=True
            ):
                note_low += "macro #[private] used in non-callback function; "
                break

        for line in non_pri_callback_set:
            if structFuncNameMatch(
                line[0], func["struct"], func["struct_trait"], func_name, path, line[1], rustle_format=True
            ):
                note_high += "missing #[private] macro for callback function; "
                break

        for line in incorrect_json_set:
            if structFuncNameMatch(
                line[0], func["struct"], func["struct_trait"], func_name, path, line[1], rustle_format=True
            ):
                note_high += line[2]
                break

        # for dc_name_line in deadcode_set:
        #     if structFuncNameMach(dc_name_line[0], func['struct'], func['struct_trait'], func_name, path):
        #         note_info += 'dead code at ' + str(dc_name_line[1]) + '; '
        #         break

        for caller in unused_ret_dict.keys():
            if structFuncNameMatch(
                caller, func["struct"], func["struct_trait"], func_name, path, None, rustle_format=True
            ):
                note_low += "call to <"
                for line, callee in unused_ret_dict[caller]:
                    note_low += callee + "(L" + line + ") "
                note_low = note_low.rstrip() + "> with unused return value; "
                break

        for func_string, file in public_interface_set:
            if structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path, file, rustle_format=True
            ):
                note_info += "public interface; "
                break

        for func_string, hasCheck in storage_gas_set:
            if hasCheck == False and structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path
            ):
                note_low += "require gas check for storage expansion; "
                break

        for func_string, hasCheck in unregistered_receiver_set:
            if hasCheck == False and structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path
            ):
                note_medium += "should panic when the receiver is not registered; "
                break

        hasPrint = False
        for func_string, func_path, func_line in unsaved_changes_set:
            if structFuncNameMatch(func_string, func["struct"], func["struct_trait"], func_name, path, func_path):
                note_high += ("" if hasPrint else "unsaved changes to map(s) at <") + "L" + str(func_line) + " "
                hasPrint = True
        if hasPrint:
            note_high = note_high.rstrip() + ">; "

        for func_string, hasCheck in unclaimed_storage_fee_set:
            if hasCheck == False and structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path
            ):
                note_low += "require balance check for storage fee; "
                break

        for func_string, hasCheck in nft_approval_check_set:
            if hasCheck == False and structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path
            ):
                note_high += "require approval_id check; "
                break

        for func_string, hasCheck in nft_owner_check_set:
            if hasCheck == False and structFuncNameMatch(
                func_string, func["struct"], func["struct_trait"], func_name, path
            ):
                note_high += "require owner check; "
                break

        with open(path, "r") as file:
            string = re.sub("//[^\n]+\n", "\n", file.read())
            for inconsistent_key in inconsistency_dict.keys():
                for match in re.compile(r"\b" + inconsistent_key + r"\b", re.MULTILINE | re.DOTALL).finditer(string):
                    line_no = string[0 : match.start()].count("\n")
                    if line2funcName(line_no, results) != func_name:
                        continue
                    note_info += (
                        "used of "
                        + inconsistent_key
                        + " at "
                        + str(line_no)
                        + " may be conflict with "
                        + str(inconsistency_dict[inconsistent_key]).replace("'", "")
                        + "; "
                    )

        note_high = note_high.strip()
        note_medium = note_medium.strip()
        note_low = note_low.strip()
        note_info = note_info.strip()

        value_matrix.append(
            [
                func_name,
                func["struct"],
                "",
                func["modifier"],
                func["macro"],
                func["visibility"],
                "working",
                func_type,
                note_high,
                note_medium,
                note_low,
                note_info,
            ]
        )
        if note_high != "" or note_medium != "" or note_low != "" or note_info != "":
            summary_value_matrix.append(
                [path[len(PROJ_PATH) :].lstrip("/"), func_name, note_high, note_medium, note_low, note_info]
            )
            json_value_matrix.append(
                [path[len(PROJ_PATH) :].lstrip("/"), func_name, note_high, note_medium, note_low, note_info]
            )

    var_results = findGlobalVar(path)
    for func_name in var_results.keys():
        value_matrix.append(
            [
                func_name,
                "",
                "",
                "N/A",
                "N/A",
                var_results[func_name]["visibility"],
                "working",
                var_results[func_name]["type"],
                "",
                "",
                "",
                "",
            ]
        )

    writer.value_matrix = value_matrix
    # writer.write_table()
    with open(
        CSV_PATH + "/near_audit-" + path.replace(PROJ_PATH, "").lstrip("/").replace("/", "-") + ".csv", "w"
    ) as file:
        writer.stream = file
        writer.write_table()

"""
add summary-only note
"""
summary_high = ""
summary_medium = ""
summary_low = ""
summary_info = ""
if len(upgrade_func_set) == 0:
    summary_low += "No upgrade function found; "
summary_medium += unique_collection_id_log
if len(unimplemented_interface_list) > 0:
    summary_medium += "Unimplemented NEP interface" + ("" if len(unimplemented_interface_list) == 1 else "s") + ": "
    for func in unimplemented_interface_list:
        summary_medium += func + ", "
    summary_medium = summary_medium[:-2] + "; "

summary_value_matrix.append(
    ["global", "global", summary_high.strip(), summary_medium.strip(), summary_low.strip(), summary_info.strip()]
)

summary_writer.value_matrix = summary_value_matrix
with open(CSV_PATH + "/summary.csv", "w") as sum_file:
    summary_writer.stream = sum_file
    summary_writer.write_table()

json_summary_writer.value_matrix = json_value_matrix
with open(CSV_PATH + "/summary.json", "w") as sum_file:
    json_summary_writer.stream = sum_file
    json_summary_writer.write_table()
