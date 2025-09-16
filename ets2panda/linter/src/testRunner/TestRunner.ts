/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import * as fs from 'node:fs';
import * as path from 'node:path';
import * as ts from 'typescript';
import { Logger } from '../lib/Logger';
import { LoggerImpl } from '../cli/LoggerImpl';
import { Command } from 'commander';
import { rimrafSync } from 'rimraf';
import { globSync } from 'glob';
import type { Path } from 'path-scurry';
import { PathScurry } from 'path-scurry';
import { createTests } from './TestFactory';
import type { LintTest } from './LintTest';
import type { TestRunnerOptions } from './TestRunnerOptions';
import type { RunTestFileOptions } from './RunTestFileOptions';
import { MIGRATE_RESULT_SUFFIX, RESULTS_DIR, TAB } from './Consts';

Logger.init(new LoggerImpl());

interface TestStatistics {
  passed: number;
  failed: number;
}

export function getSdkConfigOptions(configFile: string): ts.CompilerOptions {
  // initial configuration
  const options = ts.readConfigFile(configFile, ts.sys.readFile).config.compilerOptions;

  const allPath = ['*'];
  Object.assign(options, {
    emitNodeModulesFiles: true,
    importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Preserve,
    module: ts.ModuleKind.ES2020,
    moduleResolution: ts.ModuleResolutionKind.NodeJs,
    noEmit: true,
    target: ts.ScriptTarget.ES2021,
    baseUrl: '/',
    paths: {
      '*': allPath
    },
    lib: ['lib.es2021.d.ts'],
    types: [],
    etsLoaderPath: 'null_sdkPath'
  });

  return options;
}

function parseCommandLine(commandLineArgs: string[]): TestRunnerOptions {
  const program = new Command();
  program.
    name('testrunner').
    description('Test Runner for Linter tests').
    configureHelp({ helpWidth: 100 }).
    version('1.0.0');
  program.
    requiredOption(
      '-d, --test-dir <test_dir>',
      'Specifies a directory with test files. Multiple directories can be specified with a comma-separated list'
    ).
    option(
      '-p, --test-pattern <pattern>',
      'Specifies a glob pattern to filter the list of tests. The path from the pattern is resolved to test directory. ' +
        'Note: Only tests in specified test directory will be run, and the sub-directories will be ignored.'
    ).
    option('--sdk', 'Use SDK check logic').
    option('--interop-mode', 'Run interop-mode check');

  // method parse() eats two first args, so make them dummy
  const cmdArgs: string[] = ['dummy', 'dummy'];
  cmdArgs.push(...commandLineArgs);
  program.parse(cmdArgs);
  const programOpts = program.opts();
  const testRunnerOpts: TestRunnerOptions = {
    testDirs: processTestDirArg(programOpts.testDir),
    linterOptions: {}
  };
  if (programOpts.testPattern) {
    testRunnerOpts.testPattern = programOpts.testPattern;
  }
  if (programOpts.interopMode) {
    testRunnerOpts.linterOptions.interopCheckMode = true;
  }
  if (programOpts.sdk) {
    testRunnerOpts.linterOptions.useRtLogic = false;
  }
  return testRunnerOpts;
}

function processTestDirArg(testDirs: string | undefined): string[] {
  if (!testDirs) {
    return [];
  }
  return testDirs.
    split(',').
    map((val) => {
      return val.trim();
    }).
    filter((val) => {
      return val.length > 0;
    });
}

function runTests(): boolean {
  const testRunnerOpts = parseCommandLine(process.argv.slice(2));
  const testStats: TestStatistics = { passed: 0, failed: 0 };
  // Get tests from test directory
  for (const testDir of testRunnerOpts.testDirs) {
    if (!fs.existsSync(testDir)) {
      Logger.info(`\nThe "${testDir}" directory doesn't exist.\n`);
      continue;
    }
    Logger.info(`\nProcessing "${testDir}" directory:\n`);
    rimrafSync(path.join(testDir, RESULTS_DIR));
    let testFiles: string[] = testRunnerOpts.testPattern ?
      collectTestFilesWithGlob(testDir, testRunnerOpts.testPattern) :
      fs.readdirSync(testDir);
    testFiles = testFiles.filter((x) => {
      const file = x.trimEnd();
      return (
        (file.endsWith(ts.Extension.Ts) && !file.endsWith(ts.Extension.Dts) ||
          file.endsWith(ts.Extension.Tsx) ||
          file.endsWith(ts.Extension.Ets)) &&
        !file.endsWith(MIGRATE_RESULT_SUFFIX + path.extname(file))
      );
    });
    runTestFiles(testFiles, testDir, testRunnerOpts, testStats);
  }
  const { passed, failed } = testStats;
  Logger.info(`\nSUMMARY: ${passed + failed} total, ${passed} passed, ${failed} failed.`);
  Logger.info(failed > 0 ? '\nTEST FAILED' : '\nTEST SUCCESSFUL');

  const saveCoverageData = (): void => {
    const coverageData = globalThis.__coverage__;
    if (coverageData) {
      const projectRoot = path.resolve(__dirname, '../../..');
      const coverageDir = path.join(projectRoot, 'coverage');
      fs.mkdirSync(coverageDir, { recursive: true });

      const coverageFile = path.join(coverageDir, 'coverage.json');
      fs.writeFileSync(coverageFile, JSON.stringify(coverageData, null, 4));
    } else {
      console.log('no coverage data found');
    }
  };
  saveCoverageData();
  process.exit(failed > 0 ? -1 : 0);
}

function collectTestFilesWithGlob(testDir: string, pattern: string): string[] {
  const testDirPath = new PathScurry(testDir).cwd.fullpath();
  return globSync(pattern, {
    cwd: testDir,
    ignore: {
      childrenIgnored(p: Path): boolean {
        return p.fullpath() !== testDirPath;
      }
    }
  }).sort();
}

function runTestFiles(
  testFiles: string[],
  testDir: string,
  testRunnerOpts: TestRunnerOptions,
  testStats: TestStatistics
): void {
  for (const testFile of testFiles) {
    try {
      runTestFile({ testDir, testFile, testRunnerOpts }, testStats);
    } catch (error) {
      Logger.info(`Test ${testFile} failed:\n${TAB}` + (error as Error).message);
      testStats.failed++;
    }
  }
}

function runTestFile(runTestFileOpts: RunTestFileOptions, testStats: TestStatistics): void {
  const tests: LintTest[] = createTests(runTestFileOpts);

  tests.forEach((test: LintTest) => {
    if (test.run()) {
      ++testStats.passed;
    } else {
      ++testStats.failed;
    }
  });
}

runTests();
