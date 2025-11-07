/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import * as path from 'node:path';
import type { CommandLineOptions } from '../lib/CommandLineOptions';
import type { LinterOptions } from '../lib/LinterOptions';
import { LintTest } from './LintTest';
import type { RunTestFileOptions } from './RunTestFileOptions';
import { parseCommandLine } from '../cli/CommandLineParser';
import { getCommandLineArguments } from './CommandLineUtil';
import type { TestModeProperties } from './TestMode';
import { DEFAULT_MODE_PROPERTIES, TestMode, testModePropsMap } from './TestMode';
import type { TestArguments } from './TestArgs';
import { readTestArgsFile } from './TestArgs';
import { MigrateTest } from './MigrateTest';

interface CreateTestOptions {
  runTestFileOpts: RunTestFileOptions;
  testModeProps: TestModeProperties;
  testModeArgs?: string;
  testCommonOpts?: LinterOptions;
}

export interface CreateLintTestOptions {
  testDir: string;
  testFile: string;
  testModeProps: TestModeProperties;
  cmdOptions: CommandLineOptions;
}

export function createTests(runTestFileOpts: RunTestFileOptions): LintTest[] {
  const testArgs = readTestArgsFile(runTestFileOpts.testDir, runTestFileOpts.testFile);
  return testArgs ?
    createTestsFromTestArgs(runTestFileOpts, testArgs) :
    [createTest({ runTestFileOpts, testModeProps: DEFAULT_MODE_PROPERTIES })];
}

function createTestsFromTestArgs(runTestFileOpts: RunTestFileOptions, testArgs: TestArguments): LintTest[] {
  const tests: LintTest[] = [];
  const testCommonOpts = testArgs.commonArgs ? getLinterOptionsFromCommandLine(testArgs.commonArgs) : undefined;

  addDefaultModeIfNeeded(testArgs);

  if (testArgs.mode) {
    for (const mode of Object.keys(testArgs.mode)) {
      const testModeProps = testModePropsMap.get(mode);
      if (!testModeProps) {
        throw new Error(`Failed to create test. Unknown mode: '${mode}'`);
      }
      const testModeArgs: string | undefined = testArgs.mode[mode];
      tests.push(createTest({ runTestFileOpts, testModeProps, testCommonOpts, testModeArgs }));
    }
  }

  return tests;
}

function addDefaultModeIfNeeded(testArgs: TestArguments): void {
  if (testArgs.mode?.default === undefined) {
    // For convenience, always add 'default' mode as first
    testArgs.mode = Object.assign({ default: '' }, testArgs.mode);
  }
}

function createTest(createTestOpts: CreateTestOptions): LintTest {
  const opts = createCreateLintTestOpts(createTestOpts);
  if (createTestOpts.testModeProps.mode === TestMode.MIGRATE) {
    return new MigrateTest(opts);
  }
  return new LintTest(opts);
}

function createCreateLintTestOpts(createTestConfigOpts: CreateTestOptions): CreateLintTestOptions {
  const { runTestFileOpts, testCommonOpts, testModeArgs, testModeProps } = createTestConfigOpts;
  const { testDir, testFile, testRunnerOpts } = runTestFileOpts;

  /*
   * Test options are formed in the following order (from lowest to highest priority):
   *   - default test options;
   *   - [test_args_file] --> 'commonArgs';
   *   - [test_args_file] --> the arguments specified for a mode;
   *   - options specified by TestRunner command-line arguments;
   *   - options that enable specific mode.
   */
  const linterOpts = getDefaultTestOptions();
  if (testCommonOpts) {
    Object.assign(linterOpts, testCommonOpts);
  }
  if (testModeArgs) {
    Object.assign(linterOpts, getLinterOptionsFromCommandLine(testModeArgs));
  }
  Object.assign(linterOpts, testRunnerOpts.linterOptions);
  Object.assign(linterOpts, testModeProps.modeOpts);

  const cmdOptions: CommandLineOptions = {
    inputFiles: [path.join(testDir, testFile)],
    linterOptions: linterOpts
  };

  return {
    testDir,
    testFile,
    testModeProps,
    cmdOptions
  };
}

function getLinterOptionsFromCommandLine(cmdLine: string): LinterOptions {
  return parseCommandLine(getCommandLineArguments(cmdLine), { exitOnFail: false, disableErrorOutput: true }).
    linterOptions;
}

function getDefaultTestOptions(): LinterOptions {
  return {
    useRtLogic: true,

    /*
     * By default, treat any test file with '.ts' extension as a source file
     * (as opposed to library)
     */
    checkTsAsSource: true,
    compatibleSdkVersion: 12,
    compatibleSdkVersionStage: 'beta3',
    checkTsAndJs: true
  };
}
