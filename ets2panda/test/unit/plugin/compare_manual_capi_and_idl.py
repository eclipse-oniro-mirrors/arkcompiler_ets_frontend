#!/usr/bin/env python3
# coding: utf-8
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


import os
import sys
import re
import logging
import argparse
from pathlib import Path
from collections import defaultdict
from typing import List, DefaultDict


logging.basicConfig(stream=sys.stdout, level=logging.DEBUG, format="%(levelname)s:[%(name)s]: %(message)s", force=True)
logging.root.name = "CAPI_IDL_COMPARATOR"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare manually written CAPI against IDL")
    parser.add_argument("--header", type=str, required=True, help="Path to es2panda_lib.h")
    parser.add_argument("--idl", type=str, required=True, help="Path to es2panda_lib.idl.erb")

    local_args = parser.parse_args()
    if not os.path.isfile(local_args.header):
        raise RuntimeError(f"Bad path to es2panda_lib.h: {local_args.header}")
    if not os.path.isfile(local_args.idl):
        raise RuntimeError(f"Bad path to es2panda_lib.h: {local_args.idl}")

    return local_args


def extract_interest(data: List[str]) -> List[str]:
    start_marker = "COMPARE_MANUAL_CAPI_AND_IDL_START"
    end_marker = "COMPARE_MANUAL_CAPI_AND_IDL_END"
    start_index = -1
    end_index = -1

    for i, line in enumerate(data):
        if start_marker in line:
            if start_index != -1:
                raise RuntimeError(f"Double {start_marker}. Lines {start_index + 1} and {i + 1}")
            start_index = i
        if end_marker in line:
            if start_index == -1:
                raise RuntimeError(f"{end_marker} before {start_marker}. Line {i + 1}")
            end_index = i
            break

    if start_index == -1 or end_index == -1:
        raise RuntimeError(f"Not found markers! {start_marker} line: {start_index}. {end_marker} line: {end_index}.")

    return data[start_index + 1 : end_index]


def get_api_list_from_file(path: Path, pattern: str) -> DefaultDict[str, int]:
    store_data: DefaultDict[str, int] = defaultdict(int)
    for line in extract_interest(path.read_text(encoding="utf-8").splitlines()):
        res = re.search(pattern, line)
        if res:
            store_data[res.group(1)] += 1
    return store_data


def compare_api(header_data: DefaultDict[str, int], idl_data: DefaultDict[str, int]) -> bool:
    compare_passed = True
    total_keys = sorted(list(filter(lambda x: x.find("##") == -1, set(header_data.keys()) | set(idl_data.keys()))))

    header = "es2panda_lib.h"
    idl = "es2panda_lib.idl.erb"

    for k in total_keys:
        if not header_data.get(k):
            logging.error(f"Function '{k}' not found in {header}")
            compare_passed = False
        elif not idl_data.get(k):
            logging.error(f"Function '{k}' not found in {idl}")
            compare_passed = False
        elif header_data.get(k, 0) != idl_data.get(k, 0):
            logging.error(f"Function '{k}': {header_data.get(k, 0)} in {header}, {idl_data.get(k, 0)} in {idl}")
            compare_passed = False

    if not compare_passed:
        logging.error(f"Hint: Manually written functions in {header} must be synchronized with {idl}.")

    return not compare_passed


if __name__ == "__main__":
    args = parse_arguments()
    header_api_list = get_api_list_from_file(Path(args.header), r"\(\*(.*?)\)")
    idl_api_list = get_api_list_from_file(Path(args.idl), r"^\s*(?:\S+)\s+(\S+?)\(")
    sys.exit(compare_api(header_api_list, idl_api_list))
