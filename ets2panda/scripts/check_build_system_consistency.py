#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os
import re
import fnmatch


def parse_cmake_file_list(text, patterns):
    file_list = []
    file_dict = {}
    for match in re.finditer(r'([^\'"]*\.[a-zA-Z]+)', text):
        filename = match.group(0)
        filename = filename.split("\n")

        for file in filename:
            if file.endswith(patterns):
                file_list.append(file.strip().split("/")[-1])
                file_dict[file.strip().split("/")[-1]] = file.strip()
    return file_list, file_dict


def parse_gn_file_list(text, patterns):
    file_list = []
    file_dict = {}
    for match in re.finditer(r'[\"]([^\'"]*\.[a-zA-Z]+)[\"]', text):
        filename = match.group(1)
        if any(fnmatch.fnmatch(filename, pat) for pat in patterns):
            file_list.append(os.path.basename(filename))
            file_dict[os.path.basename(filename)] = filename
    return file_list, file_dict


def classify_files(files, files_dict):
    sources = []
    headers = []
    yamls = []
    sources_dict = {}
    headers_dict = {}
    yamls_dict = {}

    for file in files:
        if file.endswith(".cpp"):
            sources.append(file)
            sources_dict[file] = files_dict.get(file)
        elif file.endswith(".h"):
            headers.append(file)
            headers_dict[file] = files_dict.get(file)
        elif file.endswith(".yaml"):
            yamls.append(file)
            yamls_dict[file] = files_dict.get(file)

    return sources, headers, yamls, sources_dict, headers_dict, yamls_dict


def parse_root_cmake(file_path):
    with open(file_path, "r") as f:
        content1 = f.read()

    sources = []
    sources_dict = {}

    for match in re.finditer(r"set\s*\(\s*\w+\s+(.*?)\s*\)", content1, re.DOTALL):
        files, files_dict = parse_cmake_file_list(match.group(1), ".cpp")
        sources.extend(files)
        sources_dict.update(files_dict)

    return sources, sources_dict


def parse_public_cmake(file_path):
    with open(file_path, "r") as f:
        content = f.read()
    headers, yamls = [], []
    headers_dict, yamls_dict = {}, {}

    for match in re.finditer(r"set\s*\(\s*\w+\s+(.*?)\s*\)", content, re.DOTALL):
        files, files_dict = parse_cmake_file_list(match.group(1), (".h", ".yaml"))
        _, h, y, _, hd, yd = classify_files(files, files_dict)
        headers.extend(h)
        yamls.extend(y)
        headers_dict.update(hd)
        yamls_dict.update(yd)

    return headers, yamls, headers_dict, yamls_dict


def parse_gn(file_path):
    with open(file_path, "r") as f:
        content = f.read()

    sources, headers, yamls = [], [], []
    sources_dict, headers_dict, yamls_dict = {}, {}, {}

    for match in re.finditer(
        r"(libes2panda_sources|HEADERS_TO_BE_PARSED|"
        r"ES2PANDA_API_GENERATED|ES2PANDA_API|"
        r"generated_headers)\s*=\s*\[(.*?)\]",
        content,
        re.DOTALL,
    ):
        files, files_dict = parse_gn_file_list(match.group(2), ["*.cpp", "*.h", "*.yaml"])
        s, h, y, sd, hd, yd = classify_files(files, files_dict)
        sources.extend(s)
        headers.extend(h)
        yamls.extend(y)
        sources_dict.update(sd)
        headers_dict.update(hd)
        yamls_dict.update(yd)

    return sources, sources_dict, headers, headers_dict, yamls, yamls_dict


def compare_file_lists(cmake_files, cmake_files_dict, gn_files, gn_files_dict, file_type, location):
    cmake_set = set(cmake_files)
    gn_set = set(gn_files)

    only_in_cmake = cmake_set - gn_set
    only_in_gn = gn_set - cmake_set

    if only_in_cmake:
        only_in_cmake_path = []
        for file in only_in_cmake:
            only_in_cmake_path.append(cmake_files_dict[file])
        print(f"{file_type} files only exist in CMake file:", sorted(only_in_cmake_path))
        if file_type == "source file(.cpp)":
            print(
                f"please add the missing {file_type} files to libes2panda_sources in ets2panda/BUILD.gn!"
            )
        else:
            print(f"please add the missing {file_type} files to {location} in ets2panda/BUILD.gn!")
    if only_in_gn:
        only_in_gn_path = []
        for file in only_in_gn:
            only_in_gn_path.append(gn_files_dict[file])
        print(f"{file_type} files only exist in GN file:", sorted(only_in_gn_path))
        if file_type == "source file(.cpp)":
            print(f"please add the missing {file_type} files to {location} in ets2panda/CMakeList.txt!")
        else:
            print(
                f"please add the missing {file_type} files to {location} in ets2panda/public/CMakeList.txt!"
            )

    return len(only_in_cmake) == 0 and len(only_in_gn) == 0


def main():
    work_dir = sys.argv[1]
    cmake_src, cmake_src_dict = parse_root_cmake(os.path.join(work_dir, "CMakeLists.txt"))
    (
        cmake_hdr,
        cmake_yaml,
        cmake_hdr_dict,
        cmake_yaml_dict,
    ) = parse_public_cmake(os.path.join(work_dir, "public/CMakeLists.txt"))

    gn_src, gn_src_dict, gn_hdr, gn_hdr_dict, gn_yaml, gn_yaml_dict = parse_gn(
        os.path.join(work_dir, "BUILD.gn")
    )

    src_consistent = compare_file_lists(
        cmake_src, cmake_src_dict, gn_src, gn_src_dict, "*.cpp", "ES2PANDA_LIB_SRC"
    )
    hdr_consistent = compare_file_lists(
        cmake_hdr, cmake_hdr_dict, gn_hdr, gn_hdr_dict, "*.h", "HEADERS_TO-BE-PARSED"
    )
    yaml_consistent = compare_file_lists(
        cmake_yaml,
        cmake_yaml_dict,
        gn_yaml,
        gn_yaml_dict,
        "*.yaml",
        "ES2PANDA_API_GENERATED",
    )

    if src_consistent and hdr_consistent and yaml_consistent:
        print("all file types are consistent beetween CMake and GN")
        return 0
    print("Warning! inconsistent file found beetween CMake and GN")
    return 1


if __name__ == "__main__":
    exit(main())
