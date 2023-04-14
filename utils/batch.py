#!/usr/bin/env python3
import csv
import os
import re
from multiprocessing.pool import ThreadPool

repo_regex = "https://(?P<service>(\\w+\\.)+(\\w+))/(?P<user>([a-zA-Z0-9_]|\\.|-)+)/(?P<repo>([a-zA-Z0-9_]|\\.|-)+)/?"
# git_pattern = 'git@{}:{}/{}.git'
git_pattern = "https://{}/{}/{}.git"

local_repo_dir = "~/near-repo/"
local_report_dir = ".near_reports/"

os.makedirs(local_repo_dir, exist_ok=True)
os.makedirs(local_report_dir, exist_ok=True)

"""
Process csv and get the repo url data
"""
url_list = []
with open("docs/analysis/near-dapp-collection.csv", newline="") as csv_file:
    spam_reader = csv.reader(csv_file, delimiter=",")
    for row in spam_reader:
        matches = re.match(repo_regex, row[8])
        if matches != None:
            url_list.append(matches.groupdict())
print(url_list)

"""
Clone the repo
"""
for url in url_list:
    print("\n[*] Cloning {}".format(url["repo"]))
    if not os.path.isdir(local_repo_dir + url["repo"]):
        os.system(
            ("git clone " + git_pattern + " {}").format(
                url["service"], url["user"], url["repo"], local_repo_dir + url["repo"]
            )
        )
    else:
        print("[!] Repo {} already exists".format(url["repo"]))


"""
Run the analysis
"""
os.system("mkdir " + local_report_dir)


def analysis(repo):
    # repo = url['repo']
    print("\n[*] Analyzing {}".format(repo))
    os.environ["NEAR_SRC_DIR"] = local_repo_dir + repo
    # os.system('make -C ~/near_core clean_tg')
    os.system("make -C ~/near analysis > " + local_report_dir + repo + ".txt")


if True:  # use linear processing
    for repo in os.listdir("/home/xiyao/near-repo"):
        analysis(repo)
else:  # use multiprocessing
    analysis_pool = ThreadPool(16)
    analysis_pool.map(analysis, url_list)
    analysis_pool.close()
    analysis_pool.join()
