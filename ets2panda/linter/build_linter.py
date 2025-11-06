#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2025 Huawei Device Co., Ltd.
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

import argparse
import os
import shutil
import subprocess
import sys
import tarfile
import time


def copy_files(source_path, dest_path, is_file=False):
    try:
        if is_file:
            if not os.path.exists(os.path.dirname(dest_path)):
                os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            shutil.copy(source_path, dest_path)
        else:
            shutil.copytree(source_path, dest_path, dirs_exist_ok=True,
                symlinks=True)
    except Exception as err:
        raise Exception("Copy files failed. Error: " + str(err)) from err


def run_cmd(cmd, execution_path=None):
    if (cmd and cmd[0].strip().endswith('npm')):
        cmd.append('--registry')
        cmd.append('https://repo.huaweicloud.com/repository/npm/')
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                           stdin=subprocess.PIPE,
                           stderr=subprocess.PIPE,
                           cwd=execution_path)
    stdout, stderr = proc.communicate(timeout=600)
    if proc.returncode != 0:
        raise Exception(stderr.decode())
    return stdout


def run_cmd_with_retry(max_retries, wait_time, cmd, execution_path=None):
    retry_count = 0
    last_exception = None
    while retry_count < max_retries:
        try:
            run_cmd(cmd, execution_path)
            break
        except Exception as e:
            last_exception = e
            retry_count += 1
            time.sleep(wait_time)
    if retry_count >= max_retries:
        raise Exception(
            "Command failed after {r} attempts. Cmd: {c}. Error: {e}"
            .format(
                r=max_retries,
                c=" ".join(cmd),
                e=last_exception
            )
        )


def is_npm_newer_than_6(options):
    cmd = [options.npm, '-v']
    stdout = run_cmd(cmd, options.source_path)
    version_str = stdout.decode('utf-8').strip()
    # get npm major version（i.e. "6.14.15" -> 6）
    major_version = int(version_str.split('.')[0])
    if major_version is not None:
        if major_version <= 6:
            return False
        else:
            return True
    # default set to lower than v7 which can compatible with v7+
    return False


def build(options):
    build_cmd = [options.npm, 'run', 'build']
    pack_cmd = [options.npm, 'pack']
    run_cmd(build_cmd, options.source_path)
    run_cmd(pack_cmd, options.source_path)


def copy_output(options):
    run_cmd(['rm', '-rf', options.output_path])
    src = os.path.join(options.source_path, 'panda-tslinter-{}.tgz'.format(options.version))
    dest = os.path.join(options.output_path, 'panda-tslinter-{}.tgz'.format(options.version))
    copy_files(src, dest, True)
    try:
        with tarfile.open(dest, 'r:gz') as tar:
            tar.extractall(path=options.output_path)
    except tarfile.TarError as e:
        raise Exception("Error extracting files") from e
    copy_files(os.path.join(options.output_path, 'package'), options.output_path)
    run_cmd(['rm', '-rf', os.path.join(options.output_path, 'package')])
    run_cmd(['rm', '-rf', dest])
    src = os.path.join(options.source_path, 'tsconfig.json')
    dest = os.path.join(options.output_path, 'tsconfig.json')
    copy_files(src, dest, True)


def install_typescript(options):
    new_npm = is_npm_newer_than_6(options)
    tsc_file = 'file:' + options.typescript
    if new_npm:
        cmd = [options.npm, 'install', '--no-save', tsc_file, '--legacy-peer-deps', '--offline']
    else:
        cmd = [options.npm, 'install', '--no-save', tsc_file]
    run_cmd(cmd, options.source_path)


def find_files_by_prefix_suffix(directory, prefix, suffix):
    matched_files = []
    for filename in os.listdir(directory):
        if filename.startswith(prefix) and filename.endswith(suffix):
            matched_files.append(os.path.join(directory, filename))
    return sorted(matched_files, key=os.path.getctime, reverse=True)


def clean_old_packages(directory, prefix, suffix):
    res = True
    matched_files = find_files_by_prefix_suffix(directory, prefix, suffix)
    if (matched_files):
        for file in matched_files:
            try:
                os.remove(file)
            except Exception:
                res = False
    return res


def backup_package_files(source_path):
    package_name = 'package.json'
    package_back_name = 'package.json.bak'
    aa_path = os.path.join(source_path, 'arkanalyzer')
    hc_path = os.path.join(source_path, 'homecheck')
    linter_path = source_path
    copy_files(os.path.join(aa_path, package_name), os.path.join(aa_path, package_back_name), True)
    copy_files(os.path.join(hc_path, package_name), os.path.join(hc_path, package_back_name), True)
    copy_files(os.path.join(linter_path, package_name), os.path.join(linter_path, package_back_name), True)


def clean_env(source_path):
    package_name = 'package.json'
    package_back_name = 'package.json.bak'
    package_lock_name = 'package-lock.json'
    aa_path = os.path.join(source_path, 'arkanalyzer')
    hc_path = os.path.join(source_path, 'homecheck')
    linter_path = source_path
    try:
        copy_files(os.path.join(aa_path, package_back_name), os.path.join(aa_path, package_name), True)
        copy_files(os.path.join(hc_path, package_back_name), os.path.join(hc_path, package_name), True)
        copy_files(os.path.join(linter_path, package_back_name), os.path.join(linter_path, package_name), True)
        os.remove(os.path.join(hc_path, package_lock_name))
        os.remove(os.path.join(linter_path, package_lock_name))
        os.remove(os.path.join(aa_path, package_back_name))
        os.remove(os.path.join(hc_path, package_back_name))
        os.remove(os.path.join(linter_path, package_back_name))
    except Exception:
        return False
    return True


def aa_copy_lib_files(options):
    aa_path = os.path.join(options.source_path, 'arkanalyzer')
    source_file_1 = os.path.join(aa_path, 'node_modules', 'ohos-typescript', 'lib', 'lib.es5.d.ts')
    dest_path = os.path.join(aa_path, 'builtIn', 'typescript', 'api', '@internal')
    copy_files(source_file_1, dest_path, True)
    source_file_2 = os.path.join(aa_path, 'node_modules', 'ohos-typescript', 'lib', 'lib.es2015.collection.d.ts')
    copy_files(source_file_2, dest_path, True)


def hc_copy_lib_files(options):
    hc_path = os.path.join(options.source_path, 'homecheck')
    source_file = os.path.join(hc_path, 'node_modules', 'ohos-typescript', 'lib', 'lib.es5.d.ts')
    dest_path = os.path.join(hc_path, 'resources', 'internalSdk', '@internal')
    copy_files(source_file, dest_path, True)


def pack_arkanalyzer(options, new_npm):
    aa_path = os.path.join(options.source_path, 'arkanalyzer')
    tsc_file = 'file:' + options.typescript
    pack_prefix = 'arkanalyzer-'
    pack_suffix = '.tgz'
    clean_old_packages(aa_path, pack_prefix, pack_suffix)

    if new_npm:
        ts_install_cmd = [options.npm, 'install', '--no-save', tsc_file, '--legacy-peer-deps', '--offline']
    else:
        ts_install_cmd = [options.npm, 'install', '--no-save', tsc_file]
    compile_cmd = [options.npm, 'run', 'compile']
    pack_cmd = [options.npm, 'pack']
    run_cmd(ts_install_cmd, aa_path)
    aa_copy_lib_files(options)
    run_cmd(compile_cmd, aa_path)
    run_cmd(pack_cmd, aa_path)


def install_homecheck(options, max_retries, wait_time):
    new_npm = is_npm_newer_than_6(options)
    pack_arkanalyzer(options, new_npm)
    aa_path = os.path.join(options.source_path, 'arkanalyzer')
    hc_path = os.path.join(options.source_path, 'homecheck')
    aa_pack_prefix = 'arkanalyzer-'
    hc_pack_prefix = 'homecheck-'
    pack_suffix = '.tgz'
    exist_aa_packs = find_files_by_prefix_suffix(aa_path, aa_pack_prefix, pack_suffix)
    if (exist_aa_packs):
        aa_file = 'file:' + exist_aa_packs[0]
        if new_npm:
            aa_install_cmd = [options.npm, 'install', aa_file, '--legacy-peer-deps', '--offline']
        else:
            aa_install_cmd = [options.npm, 'install', aa_file]
        run_cmd_with_retry(max_retries, wait_time, aa_install_cmd, hc_path)
    else:
        raise Exception('Failed to find arkanalyzer npm package')

    clean_old_packages(hc_path, hc_pack_prefix, pack_suffix)
    tsc_file = 'file:' + options.typescript
    if new_npm:
        ts_install_cmd = [options.npm, 'install', '--no-save', tsc_file, '--legacy-peer-deps', '--offline']
    else:
        ts_install_cmd = [options.npm, 'install', '--no-save', tsc_file]
    pack_cmd = [options.npm, 'pack']
    compile_cmd = [options.npm, 'run', 'compile']
    run_cmd_with_retry(max_retries, wait_time, ts_install_cmd, hc_path)
    hc_copy_lib_files(options)
    run_cmd(compile_cmd, hc_path)
    run_cmd(pack_cmd, hc_path)
    exist_hc_packs = find_files_by_prefix_suffix(hc_path, hc_pack_prefix, pack_suffix)
    if (exist_hc_packs):
        hc_file = 'file:' + exist_hc_packs[0]
        if new_npm:
            hc_install_cmd = [options.npm, 'install', hc_file, '--legacy-peer-deps', '--offline']
        else:
            hc_install_cmd = [options.npm, 'install', hc_file]
        run_cmd_with_retry(max_retries, wait_time, hc_install_cmd, options.source_path)
    else:
        raise Exception('Failed to find homecheck npm package')


def extract(package_path, dest_path, package_name):
    try:
        with tarfile.open(package_path, 'r:gz') as tar:
            tar.extractall(path=dest_path)
    except tarfile.TarError as e:
        raise Exception("Error extracting files") from e
    dest_package_path = os.path.join(dest_path, package_name)
    if (os.path.exists(dest_package_path)):
        shutil.rmtree(dest_package_path)
    os.rename(os.path.join(dest_path, 'package'), dest_package_path)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--npm', help='path to a npm exetuable')
    parser.add_argument('--source-path', help='path to build system source')
    parser.add_argument('--output-path', help='path to output')
    parser.add_argument('--typescript', help='path to typescript')
    parser.add_argument('--version', help='linter version')

    options = parser.parse_args()
    return options


def main():
    options = parse_args()
    backup_package_files(options.source_path)
    install_homecheck(options, 5, 3)
    install_typescript(options)
    node_modules_path = os.path.join(options.source_path, "node_modules")
    extract(options.typescript, node_modules_path, "typescript")
    build(options)
    copy_output(options)
    clean_env(options.source_path)


if __name__ == '__main__':
    lock_file_path = "./rule-config.json"
    lock_file = open(lock_file_path, "w")
    acquired = False

    try:
        if os.name == "posix":
            import fcntl
            fcntl.flock(lock_file, fcntl.LOCK_EX)
            acquired = True
        elif os.name == "nt":
            import msvcrt
            while True:
                try:
                    msvcrt.locking(lock_file.fileno(), 0, 1)
                    acquired = True
                    break
                except IOError:
                    time.sleep(0.1)
        if acquired:
            main()
    finally:
        if acquired and os.name == "nt":
            msvcrt.locking(lock_file.fileno(), 1, 1)
        lock_file.close()
