#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Collect built packages into the artifacts directory.

For each package, only the entries listed in its package.json "files" field are
copied (plus package.json). From node_modules, only the top-level entries on the
package's whitelist (loaded from a JSON file) are copied. A package that is not
in the whitelist is an error.
"""

import argparse
import json
import os
import shutil
import sys


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--workspace",
        required=True,
        help="workspace directory that contains the built packages",
    )
    parser.add_argument(
        "--dest",
        required=True,
        help="destination artifacts directory",
    )
    parser.add_argument(
        "--whitelist",
        required=True,
        help="path to the node_modules whitelist JSON file",
    )
    parser.add_argument(
        "packages",
        nargs="+",
        help="package names to collect from the workspace",
    )
    return parser.parse_args()


def load_whitelist(path):
    with open(path) as handler:
        return json.load(handler)


def copy_tree(source_path, destination_path):
    shutil.copytree(
        source_path,
        destination_path,
        dirs_exist_ok=True,
        symlinks=True,
    )


def copy_file(source_path, destination_path):
    os.makedirs(os.path.dirname(destination_path), exist_ok=True)
    shutil.copy2(source_path, destination_path, follow_symlinks=False)


def read_files_field(source_dir):
    with open(os.path.join(source_dir, "package.json")) as handler:
        return [
            entry
            for entry in json.load(handler).get("files", [])
            if not entry.startswith("!")
        ]


def collect_package(workspace, dest, package, whitelist):
    source_dir = os.path.join(workspace, package)
    destination_dir = os.path.join(dest, package)
    shutil.rmtree(destination_dir, ignore_errors=True)
    os.makedirs(destination_dir, exist_ok=True)

    # package.json is always part of an npm package.
    copy_file(
        os.path.join(source_dir, "package.json"),
        os.path.join(destination_dir, "package.json"),
    )

    for entry in read_files_field(source_dir):
        source_entry = os.path.join(source_dir, entry)
        destination_entry = os.path.join(destination_dir, entry)
        if os.path.isdir(source_entry):
            copy_tree(source_entry, destination_entry)
        elif os.path.isfile(source_entry):
            copy_file(source_entry, destination_entry)

    if package not in whitelist:
        raise KeyError(
            "no node_modules whitelist entry for package '{}'".format(package)
        )
    node_modules = os.path.join(source_dir, "node_modules")
    if not os.path.isdir(node_modules):
        return
    destination_node_modules = os.path.join(destination_dir, "node_modules")
    os.makedirs(destination_node_modules, exist_ok=True)
    for entry in whitelist[package]:
        source_entry = os.path.join(node_modules, entry)
        if os.path.isdir(source_entry):
            copy_tree(
                source_entry,
                os.path.join(destination_node_modules, entry),
            )


def main():
    options = parse_args()
    whitelist = load_whitelist(options.whitelist)
    for package in options.packages:
        collect_package(options.workspace, options.dest, package, whitelist)
    return 0


if __name__ == "__main__":
    sys.exit(main())
