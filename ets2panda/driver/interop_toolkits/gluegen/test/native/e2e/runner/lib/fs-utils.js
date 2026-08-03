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

'use strict';

const fs = require('fs');
const path = require('path');
const os = require('os');

// Lists immediate sub-directories of `dir` (empty array if `dir` doesn't exist), sorted by name so
// case discovery order -- and therefore report order -- is stable across runs/platforms.
function listSubdirectoriesSorted(dir) {
  if (!fs.existsSync(dir)) {
    return [];
  }
  return fs
    .readdirSync(dir, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name)
    .sort();
}

// Default `inputFiles` discovery for a case that doesn't specify one explicitly in case.json:
// every `*.ets` file directly under `<caseDir>/input/`, sorted by name, returned as paths relative
// to `caseDir` (e.g. "input/a.ets") -- the same relative form `case.json`'s explicit `inputFiles`
// uses, so both paths through case-runner.js can be treated uniformly.
function discoverDefaultInputFiles(caseDir) {
  const inputDir = path.join(caseDir, 'input');
  if (!fs.existsSync(inputDir)) {
    return [];
  }
  return fs
    .readdirSync(inputDir)
    .filter((name) => name.endsWith('.ets'))
    .sort()
    .map((name) => path.posix.join('input', name));
}

// Copies `srcPath` to `destPath`, creating `destPath`'s parent directory first. No-op (returns
// false) if `srcPath` doesn't exist -- used by case-runner.js's negative "referenced source file
// is missing" case, where the whole point is that the path never resolves to a real file.
function copyIfExists(srcPath, destPath) {
  if (!fs.existsSync(srcPath)) {
    return false;
  }
  fs.mkdirSync(path.dirname(destPath), { recursive: true });
  fs.copyFileSync(srcPath, destPath);
  return true;
}

// Recursively copies the entire `input/` subdirectory of `caseDir` into `scratchDir`,
// so scratch directory contains every file under input/ (including prefabs / dynamic
// modules that the test case's .ets sources may reference via arktsconfig paths).
function copyInputDirectory(caseDir, scratchDir) {
  const srcInputDir = path.join(caseDir, 'input');
  const destInputDir = path.join(scratchDir, 'input');
  if (!fs.existsSync(srcInputDir)) {
    return;
  }
  fs.cpSync(srcInputDir, destInputDir, { recursive: true });
}

// Resolves each path in `inputFilesRel` (relative to `caseDir`) to its corresponding
// path under `scratchDir`, returning the scratch-absolute paths for the input-file-list.
function resolveScratchPaths(inputFilesRel, scratchDir) {
  return inputFilesRel.map((relPath) => path.join(scratchDir, relPath));
}

function makeScratchDir(prefix) {
  return fs.mkdtempSync(path.join(os.tmpdir(), `${prefix}-`));
}

// Removes `dir` and everything under it, unless GLUEGEN_E2E_KEEP_TMP is set (handy when debugging
// a failing case: rerun with that env var and inspect the scratch dir the failure message prints).
function removeScratchDir(dir) {
  if (process.env.GLUEGEN_E2E_KEEP_TMP) {
    return;
  }
  fs.rmSync(dir, { recursive: true, force: true });
}

module.exports = {
  listSubdirectoriesSorted,
  discoverDefaultInputFiles,
  copyIfExists,
  copyInputDirectory,
  resolveScratchPaths,
  makeScratchDir,
  removeScratchDir,
};
