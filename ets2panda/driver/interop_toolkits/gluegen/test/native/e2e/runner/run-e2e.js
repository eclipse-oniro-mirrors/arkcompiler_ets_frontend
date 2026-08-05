#!/usr/bin/env node
/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// End-to-end test runner for gluegen: spawns the real, built `gluegen` executable (as opposed to
// test/native/unit/, which links gluegen's sources directly into a gtest binary and calls
// Gluegen::Run() in-process) against each case directory under --positive-cases-dir /
// --negative-cases-dir, and checks the result. See ../README.md for the case.json schema.
//
// Usage (see CMakeLists.txt for the exact invocation ctest uses):
//   node run-e2e.js \
//     --gluegen-binary /path/to/built/gluegen \
//     --default-arktsconfig /path/to/bin-gtests/arktsconfig.json \
//     --positive-cases-dir /path/to/positive-cases \
//     --negative-cases-dir /path/to/negative-cases \
//     [--run-prefix "qemu-aarch64 -L /sysroot"] \
//     [--update-golden]

'use strict';

const path = require('path');

const { parseArgs } = require('./lib/cli-args');
const { listSubdirectoriesSorted } = require('./lib/fs-utils');
const { runCase } = require('./lib/case-runner');

function fail(message) {
  console.error(`gluegen e2e runner: ${message}`);
  process.exit(1);
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  for (const required of ['gluegen-binary', 'default-arktsconfig', 'positive-cases-dir', 'negative-cases-dir']) {
    if (!args[required]) {
      fail(`missing required --${required}`);
    }
  }

  const gluegenBinary = path.resolve(args['gluegen-binary']);
  const defaultArktsconfig = path.resolve(args['default-arktsconfig']);
  const updateGolden = Boolean(args['update-golden']);
  // Optional: CMake's PANDA_RUN_PREFIX (e.g. a qemu wrapper for a cross-compiled `gluegen`);
  // empty/omitted for a native host build. See lib/case-runner.js's runGluegen().
  const runPrefix = typeof args['run-prefix'] === 'string' ? args['run-prefix'] : '';

  const suites = [
    { kind: 'positive', dir: path.resolve(args['positive-cases-dir']) },
    { kind: 'negative', dir: path.resolve(args['negative-cases-dir']) },
  ];

  const results = [];
  for (const suite of suites) {
    for (const caseName of listSubdirectoriesSorted(suite.dir)) {
      results.push(
        runCase({
          caseDir: path.join(suite.dir, caseName),
          name: `${suite.kind}/${caseName}`,
          kind: suite.kind,
          gluegenBinary,
          runPrefix,
          defaultArktsconfig,
          updateGolden,
        })
      );
    }
  }

  if (results.length === 0) {
    fail('no test cases discovered under --positive-cases-dir / --negative-cases-dir');
  }

  let failureCount = 0;
  for (const result of results) {
    if (result.ok) {
      console.log(`[PASS] ${result.name}${result.message ? ` -- ${result.message}` : ''}`);
    } else {
      failureCount++;
      console.error(`[FAIL] ${result.name}${result.description ? ` (${result.description})` : ''}`);
      console.error(result.message.split('\n').map((line) => `       ${line}`).join('\n'));
    }
  }

  console.log(`\n${results.length - failureCount}/${results.length} gluegen e2e cases passed`);
  process.exit(failureCount === 0 ? 0 : 1);
}

main();
