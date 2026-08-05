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
const { spawnSync } = require('child_process');

const { discoverDefaultInputFiles, copyIfExists, copyInputDirectory, resolveScratchPaths, makeScratchDir,
  removeScratchDir } = require('./fs-utils');
const { normalizeOutputJson, deepEqual, formatDiff } = require('./json-compare');

// Reads `<caseDir>/case.json` if present, otherwise returns `{}` -- every field is optional and
// defaulted by runCase() below, so a case with no case.json at all is valid (see
// positive-cases/basic-class-and-function, which relies entirely on defaults).
function readManifest(caseDir) {
  const manifestPath = path.join(caseDir, 'case.json');
  if (!fs.existsSync(manifestPath)) {
    return {};
  }
  return JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
}

// Resolves case.json's `args.arktsconfig` (or its default) to an absolute path to pass via
// `--arktsconfig`, or `undefined` if the flag should be omitted entirely.
//   - unset / "default" -> `defaultArktsconfig` (the real stdlib-backed fixture built by
//     ets2panda's own CMakeLists.txt, passed in from run-e2e.js's --default-arktsconfig).
//   - null -> omit --arktsconfig (used by negative-cases to exercise "arktsconfig not specified").
//   - any other string -> resolved relative to caseDir (used by negative-cases to point at a
//     deliberately-missing or malformed arktsconfig.json living next to the case).
function resolveArktsconfigArg(manifest, caseDir, defaultArktsconfig) {
  const setting = manifest.args && 'arktsconfig' in manifest.args ? manifest.args.arktsconfig : 'default';
  if (setting === null) {
    return undefined;
  }
  if (setting === 'default') {
    return defaultArktsconfig;
  }
  return path.join(caseDir, setting);
}

// Copies every input file this case references into `scratchDir` (preserving their "input/..."
// relative path) and returns their scratch-absolute paths, in order, for the --input-file-list
// file. Files that don't actually exist under `caseDir` are still included by path (not copied) --
// this is what lets a negative-case reference a source file that was never created, to exercise
// gluegen's "source file does not exist" failure.
function materializeInputFiles(inputFilesRel, caseDir, scratchDir) {
  return inputFilesRel.map((relPath) => {
    const srcPath = path.join(caseDir, relPath);
    const destPath = path.join(scratchDir, relPath);
    copyIfExists(srcPath, destPath);
    return destPath;
  });
}

// `runPrefix` mirrors CMake's PANDA_RUN_PREFIX (e.g. "qemu-aarch64 -L /sysroot" for a
// cross-compiled `gluegen`, empty for a native host build) -- see the toolchain files under
// runtime_core/*/cmake/toolchain/cross-*-qemu-*.cmake, and how ets2panda's own
// test/tsconfig/*/CMakeLists.txt pass it through to their test drivers. When non-empty, its first
// whitespace-separated token becomes the actual spawned executable, with the rest of the prefix
// and `gluegenBinary` prepended to gluegen's own argv.
function runGluegen(gluegenBinary, runPrefix, { inputFileListPath, arktsconfigPath, outputPath, cachePath, reportPath }) {
  const argv = ['--input-file-list', inputFileListPath];
  if (arktsconfigPath !== undefined) {
    argv.push('--arktsconfig', arktsconfigPath);
  }
  argv.push('--output', outputPath);
  if (cachePath !== undefined) {
    argv.push('--cache-path', cachePath);
  }
  // Always request a structured diagnostics report -- negative-cases uses it (see
  // checkNegativeOutcome) to assert on DiagnosticEngine's actual code/severity, not just prose
  // matched out of stderr.
  argv.push('--report-path', reportPath);

  const prefixParts = runPrefix ? runPrefix.trim().split(/\s+/) : [];
  if (prefixParts.length === 0) {
    return spawnSync(gluegenBinary, argv, { encoding: 'utf8' });
  }
  const [prefixBinary, ...prefixArgs] = prefixParts;
  return spawnSync(prefixBinary, [...prefixArgs, gluegenBinary, ...argv], { encoding: 'utf8' });
}

// Runs a single positive-cases (or negative-cases) directory end-to-end: builds a scratch copy of
// its inputs, spawns the real `gluegen` executable against them, and checks the result against
// case.json's `expect` (or this case type's defaults). Returns `{ name, description, ok, message }`
// -- never throws; any unexpected error is caught and reported as a failed case so one broken case
// can't abort the whole run.
function runCase({ caseDir, name, kind, gluegenBinary, runPrefix, defaultArktsconfig, updateGolden }) {
  const manifest = readManifest(caseDir);
  const description = manifest.description || '';
  const isPositive = kind === 'positive';
  let scratchDir;
  try {
    scratchDir = makeScratchDir('gluegen-e2e');
    const inputFilesRel = manifest.inputFiles || discoverDefaultInputFiles(caseDir);
    copyInputDirectory(caseDir, scratchDir);
    const inputFilePaths = resolveScratchPaths(inputFilesRel, scratchDir);
    const inputFileListPath = path.join(scratchDir, 'input-file-list.txt');
    fs.writeFileSync(inputFileListPath, inputFilePaths.join('\n') + '\n');

    const arktsconfigPath = resolveArktsconfigArg(manifest, caseDir, defaultArktsconfig);
    const outputPath = path.join(scratchDir, 'output.json');
    const reportPath = path.join(scratchDir, 'report.json');
    const cachePath =
      manifest.args && manifest.args.cachePath === 'auto' ? path.join(scratchDir, 'cache') : undefined;
    const result = runGluegen(gluegenBinary, runPrefix, {
      inputFileListPath,
      arktsconfigPath,
      outputPath,
      cachePath,
      reportPath,
    });

    const expect = manifest.expect || {};
    const expectedExitCode = typeof expect.exitCode === 'number' ? expect.exitCode : isPositive ? 0 : 1;
    if (result.status !== expectedExitCode) {
      return {
        name,
        description,
        ok: false,
        message:
          `expected exit code ${expectedExitCode}, got ${result.status}` +
          ` (signal: ${result.signal})\nstdout:\n${result.stdout}\nstderr:\n${result.stderr}`,
      };
    }

    if (isPositive) {
      return checkPositiveOutcome({ caseDir, expect, outputPath, scratchDir, updateGolden, name, description });
    }
    return checkNegativeOutcome({ expect, outputPath, reportPath, result, name, description });
  } catch (err) {
    return { name, description, ok: false, message: `unexpected error: ${err.stack || err}` };
  } finally {
    if (scratchDir) {
      removeScratchDir(scratchDir);
    }
  }
}

function checkPositiveOutcome({ caseDir, expect, outputPath, scratchDir, updateGolden, name, description }) {
  const expectedOutputRel = 'outputFile' in expect ? expect.outputFile : 'expected/output.json';
  if (!expectedOutputRel) {
    // Case explicitly opts out of output comparison (expect.outputFile: null); exit code
    // (already checked) is all that's asserted.
    return { name, description, ok: true, message: '' };
  }
  if (!fs.existsSync(outputPath)) {
    return { name, description, ok: false, message: `gluegen exited 0 but did not create --output at all` };
  }
  const actualJson = normalizeOutputJson(JSON.parse(fs.readFileSync(outputPath, 'utf8')), scratchDir);
  const expectedOutputPath = path.join(caseDir, expectedOutputRel);

  if (updateGolden) {
    fs.mkdirSync(path.dirname(expectedOutputPath), { recursive: true });
    fs.writeFileSync(expectedOutputPath, `${JSON.stringify(actualJson, null, 2)}\n`);
    return { name, description, ok: true, message: `golden file updated: ${expectedOutputPath}` };
  }

  if (!fs.existsSync(expectedOutputPath)) {
    return {
      name,
      description,
      ok: false,
      message: `missing golden file ${expectedOutputPath} (rerun with --update-golden to create it)`,
    };
  }
  const expectedJson = JSON.parse(fs.readFileSync(expectedOutputPath, 'utf8'));
  if (!deepEqual(actualJson, expectedJson)) {
    return { name, description, ok: false, message: formatDiff(actualJson, expectedJson) };
  }
  return { name, description, ok: true, message: '' };
}

// Checks case.json's `expect.diagnostics` (an array of `{ code, severity }`) against the
// structured report gluegen wrote via `--report-path` -- i.e. DiagnosticEngine's own actual
// records, rather than prose scraped out of stderr. Returns an error message string, or `null` if
// every expected diagnostic was found. `report.json`'s shape is
// `{"diagnostics": {"warnings": [Diagnostic, ...], "errors": [Diagnostic, ...]}}` (see
// Gluegen::WriteDiagnosticsReport); each `Diagnostic` carries a string `code` (see
// DiagnosticCode in diagnostic.hpp).
function checkDiagnostics(expectedDiagnostics, reportPath) {
  if (!fs.existsSync(reportPath)) {
    return `expected a diagnostics report at ${reportPath}, but gluegen did not write one`;
  }
  const report = JSON.parse(fs.readFileSync(reportPath, 'utf8'));
  const bySeverity = (report.diagnostics && report.diagnostics) || {};
  for (const expected of expectedDiagnostics) {
    const bucket = (bySeverity[`${expected.severity}s`] || []);
    const found = bucket.some((diagnostic) => String(diagnostic.code) === String(expected.code));
    if (!found) {
      return (
        `expected a ${expected.severity} diagnostic with code ${expected.code} in ${reportPath}, got:\n` +
        `${JSON.stringify(report, null, 2)}`
      );
    }
  }
  return null;
}

function checkNegativeOutcome({ expect, outputPath, reportPath, result, name, description }) {
  // A negative case must not leave behind a usable --output: gluegen failing "halfway" and still
  // producing a glue JSON would let a downstream build step silently consume stale/partial data.
  if (fs.existsSync(outputPath)) {
    return { name, description, ok: false, message: `gluegen exited non-zero but still created --output` };
  }
  if (expect.stderrContains && !result.stderr.includes(expect.stderrContains)) {
    return {
      name,
      description,
      ok: false,
      message: `expected stderr to contain ${JSON.stringify(expect.stderrContains)}, got:\n${result.stderr}`,
    };
  }
  // Regex counterpart to stderrContains, for messages that embed environment-dependent absolute
  // paths (e.g. a scratch dir or the checkout location) where a fixed substring can't be hardcoded
  // into case.json -- e.g. "Arktsconfig file does not exist: .*no-such-arktsconfig\\.json$".
  // Applied with no implicit flags (case-sensitive, "." does not match newlines); pass an explicit
  // "(?s)"/inline-flag style pattern if that's ever needed.
  if (expect.stderrMatches && !new RegExp(expect.stderrMatches).test(result.stderr)) {
    return {
      name,
      description,
      ok: false,
      message: `expected stderr to match /${expect.stderrMatches}/, got:\n${result.stderr}`,
    };
  }
  // The primary, wording-independent check: assert against DiagnosticEngine's own structured
  // report rather than (only) matching prose out of stderr, so renaming/rewording a diagnostic's
  // description doesn't spuriously break this suite.
  if (expect.diagnostics) {
    const message = checkDiagnostics(expect.diagnostics, reportPath);
    if (message) {
      return { name, description, ok: false, message };
    }
  }
  return { name, description, ok: true, message: '' };
}

module.exports = { runCase };
