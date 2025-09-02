#!/usr/bin/env python3
# coding=utf-8
#
# Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

import logging
import os
import re
import sys
import git

logging.basicConfig(stream=sys.stdout, level=logging.INFO)

LINTER_PATH = 'ets2panda/linter'

merge_re = re.compile(r'Merge pull request !')
revert_re = re.compile(r'This reverts commit (.+)\.')
cherry_pick_re = re.compile(r'\(cherry picked from commit (.+)\)')


def get_branch_map(repo, branch_name, path):
    result = {}

    commits = list(repo.iter_commits(branch_name, path))
    commits.reverse()
    for commit in commits:
        msg = commit.message
        if merge_re.search(msg) is not None:
            continue

        revert_match = revert_re.search(msg)
        if revert_match is not None:
            sha = revert_match.group(1)
            if sha in result:
                del result[sha]
                continue

        cherry_pick_match = cherry_pick_re.search(msg)
        sha = commit.hexsha if cherry_pick_match is None else cherry_pick_match.group(1)
        if sha not in result:
            result[sha] = commit
        else:
            raise Exception(f'Duplicate commit {sha}')

    return result


def print_complement(of, to):
    logging.info('-' * 40)
    for sha in to:
        if sha not in of:
            commit = to[sha]
            logging.info('{}\n\n{}', commit.hexsha, commit.message.strip('\n'))
            logging.info('-' * 40)


def main():
    if len(sys.argv) != 3:
        logging.info('Usage:\n{} first_branch_name second_branch_name', sys.argv[0])
        return -1

    first_branch_name = sys.argv[1]
    second_branch_name = sys.argv[2]

    repo = git.Repo(os.getcwd() + '/../..')
    first_branch_map = get_branch_map(repo, first_branch_name, LINTER_PATH)
    second_branch_map = get_branch_map(repo, second_branch_name, LINTER_PATH)

    logging.info('Commits in `{}`, but not in `{}`:\n', first_branch_name, second_branch_name)
    print_complement(second_branch_map, first_branch_map)

    logging.info('\n')
    logging.info('=' * 80)
    logging.info('\n')

    logging.info('Commits in `{}`, but not in `{}`:\n', second_branch_name, first_branch_name)
    print_complement(first_branch_map, second_branch_map)

    return 0

if __name__ == '__main__':
    sys.exit(main())
