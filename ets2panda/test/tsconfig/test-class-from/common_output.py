#!/usr/bin/env python3
# coding=utf-8
#
# Copyright (c) 2026 Huawei Device Co., Ltd.
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
import re
import copy
import json


def _load_common_data(expected_path):
    expected_dir = os.path.dirname(os.path.abspath(expected_path))
    data_dir = os.path.dirname(expected_dir)

    with open(os.path.join(data_dir, "common_expected.json"), "r", encoding="utf-8") as data_file:
        data = json.load(data_file)
        return data


def get_common_output(expected_file_path):
    return _load_common_data(expected_file_path)


def remove_common_part(actual_output, common_outputs, part_name):
    result = actual_output

    if part_name in common_outputs:
        result = re.sub(common_outputs[part_name], "", actual_output)

    return result