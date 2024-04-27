#/bin/python

import os
from urllib.request import urlopen
import json


repo = os.environ.get("GITHUB_REPOSITORY")


def create_release_note():
    release_notes = {}
    latest_version = ""
    with open("NEWS") as f:
        for line in f:
            if line.startswith("["):
                version = line.strip()
                version = version.replace("[", "")
                version = version.replace("]", "")
                version = version.strip()
                release_notes[version] = ""
                latest_version = version
            else:
                release_notes[latest_version]+= line.strip() + "\n"
    
    return release_notes

def read_latest_release():
    url = f"https://api.github.com/repos/{repo}/releases"
    print(url)
    response = urlopen(url)
    data = response.read()
    last_release = json.loads(data)[0]
    return last_release

def current_version():
    cmake_file = ""
    with open("CMakeLists.txt") as f:
        cmake_file = f.read()
    version = ""
    #project(rcssmonitor VERSION
    cmake_file = cmake_file.split("project(rcssmonitor VERSION")[1]
    version = cmake_file.split(")")[0]
    version = version.strip()
    return version
        
def compare_version(version1, version2):
    version1 = version1.split(".")
    version2 = version2.split(".")
    for i in range(0, len(version1)):
        if int(version1[i]) > int(version2[i]):
            return 1
        elif int(version1[i]) < int(version2[i]):
            return -1
    return 0
def release_note_until_last_release(release_notes, last_release):  
    out = ""  
    for key in release_notes:
        if compare_version(key, last_release) > 0:
            out+= f"[{key}]\n"
            out += release_notes[key]
            out += "\n\n"
        elif compare_version(key, last_release) == 0:
            break
    return out

def get_release_from_github(latest_release):
    out = latest_release["tag_name"]
    out = out.replace("rcssmonitor-", "")
    return out
    

def main():

    release_notes = create_release_note()
    last_release_org = read_latest_release()
    last_release = get_release_from_github(last_release_org)
    curr_version = current_version()
    print(f"Current version: {curr_version}")
    print(f"Action release: {last_release}")
    release = release_note_until_last_release(release_notes, last_release)
    with open("release_note.md", "w") as f:
        if release == "":
            f.write("TBD")
            return
        f.write(release)
    
if __name__ == "__main__":
    main()