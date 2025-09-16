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


import argparse
import os


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Cpp headers parser to .yaml")

    parser.add_argument("--mode", "-m", type=str, required=True, help="Mode: 'static' or 'dynamic'")
    parser.add_argument("--es2panda", "-e", type=str, required=True, help="Path to current es2panda")
    parser.add_argument("--es2panda-pre-merge", "-c", type=str, required=False, help="Path to pre_merge es2panda")
    parser.add_argument("--test-dir", "-t", type=str, required=True, help="Path to test directory with test files")
    parser.add_argument("--work-dir", "-a", type=str, required=True, help="Path to the working temp folder")
    parser.add_argument("--werror", "-w", action="store_true", help="Warnings as errors")
    parser.add_argument(
        "--dynamic-regression",
        "-d",
        type=float,
        required=False,
        default=0.05,
        help="Acceptable regression compared to the pre_merge",
    )
    parser.add_argument(
        "--static-regression",
        "-s",
        type=float,
        required=False,
        default=0.1,
        help="Acceptable regression compared to static measurement",
    )
    parser.add_argument("--runs", "-n", type=int, required=False, default=25, help="Number of times to run the command")

    return parser.parse_args()


def check_arguments(args: argparse.Namespace) -> argparse.Namespace:
    if args.mode not in ["static", "dynamic"]:
        raise RuntimeError(f"Invalid mode: {args.mode}\nSee --help for more.")
    if not os.path.isfile(args.es2panda):
        raise RuntimeError(f"Bad path to current es2panda: {args.es2panda}\nSee --help for more.")
    if args.mode == "dynamic" and not os.path.isfile(args.es2panda_pre_merge):
        raise RuntimeError(f"Bad path to pre_merge es2panda: {args.es2panda_pre_merge}\nSee --help for more.")
    if not os.path.isdir(args.test_dir):
        raise RuntimeError(f"Bad path to test_dir: {args.test_dir}\nSee --help for more.")
    if args.dynamic_regression > 1 or args.dynamic_regression < -1:
        raise RuntimeError(
            f"Static regression must be in value range [-1, 1], current: {args.dynamic_regression}\n"
            "See --help for more."
        )
    if args.static_regression > 1 or args.static_regression < -1:
        raise RuntimeError(
            f"Static regression must be in value range [-1, 1], current: {args.static_regression}\n"
            "See --help for more."
        )

    return args
