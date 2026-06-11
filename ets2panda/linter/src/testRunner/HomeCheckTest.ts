/*
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

import { spawnSync } from 'node:child_process';
import * as path from 'node:path';
import type { LintRunResult } from '../lib/LintRunResult';
import { Logger } from '../lib/Logger';
import type { ProblemInfo } from '../lib/ProblemInfo';
import type { TestModeProperties } from './TestMode';
import { TestMode } from './TestMode';
import type { HomeCheckArguments } from './TestArgs';
import type { RunTestFileOptions } from './RunTestFileOptions';
import { LintTest } from './LintTest';
import { TAB } from './Consts';
import type { LinterOptions } from '../lib/LinterOptions';
import { getCommandLineArguments } from './CommandLineUtil';
import { parseCommandLine } from '../cli/CommandLineParser';

export class HomeCheckTest extends LintTest {
  private readonly homecheckArgs: HomeCheckArguments;
  private readonly testCommonOpts?: LinterOptions;
  private readonly testModeArgs?: string;

  constructor(
    runTestFileOpts: RunTestFileOptions,
    homecheckArgs: HomeCheckArguments,
    testModeProps: TestModeProperties,
    testCommonOpts?: LinterOptions,
    testModeArgs?: string
  ) {
    const testFilePath = path.normalize(path.join(runTestFileOpts.testDir, runTestFileOpts.testFile));
    const linterOptions = createHomeCheckLinterOptions(testModeProps, testCommonOpts, testModeArgs, runTestFileOpts);

    super({
      testDir: runTestFileOpts.testDir,
      testFile: runTestFileOpts.testFile,
      testModeProps: testModeProps,
      cmdOptions: {
        inputFiles: [testFilePath],
        linterOptions: linterOptions
      }
    });
    this.homecheckArgs = homecheckArgs;
    this.testCommonOpts = testCommonOpts;
    this.testModeArgs = testModeArgs;
  }

  run(): boolean {
    Logger.info(`Running test ${this.testFile} (${TestMode[this.testModeProps.mode]} HomeCheck mode)`);
    const runnerResult = this.runHomeCheck();
    if (!runnerResult) {
      return false;
    }
    const problemsInfos = new Map([[path.normalize(this.cmdOptions.inputFiles[0]), runnerResult.problems]]);
    return this.validate({ problemsInfos } as LintRunResult);
  }

  private runHomeCheck(): { problems: ProblemInfo[] } | null {
    const projectPath = path.resolve(this.testDir, this.homecheckArgs.projectPath ?? '.');
    const testFilePath = path.normalize(path.resolve(this.testDir, this.testFile));
    const runnerConfig = {
      projectPath,
      testFile: testFilePath,
      rules: this.homecheckArgs.rules,
      ruleSet: this.homecheckArgs.ruleSet,
      ruleOptions: this.resolveRuleOptions(),
      ohosSdkPath: this.resolveOptionalTestPath(this.homecheckArgs.ohosSdkPath),
      hmsSdkPath: this.resolveOptionalTestPath(this.homecheckArgs.hmsSdkPath),
      languageTags: this.resolveLanguageTags(projectPath),
      modeOpts: this.testModeProps.modeOpts
    };
    const encodedConfig = Buffer.from(JSON.stringify(runnerConfig)).toString('base64');
    const runnerPath = path.join(__dirname, 'HomeCheckRunner.js');
    const result = spawnSync(process.execPath, [runnerPath, encodedConfig], {
      cwd: process.cwd(),
      encoding: 'utf8'
    });
    if (result.status !== 0) {
      Logger.info(`${TAB}HomeCheck test failed to run:\n${result.stderr || result.stdout}`);
      return null;
    }
    try {
      return JSON.parse(result.stdout);
    } catch (error) {
      Logger.info(`${TAB}Failed to parse HomeCheck test result: ${(error as Error).message}`);
      Logger.info(`${TAB}Raw output:\n${result.stdout}`);
      return null;
    }
  }

  private resolveOptionalTestPath(filePath: string | undefined): string {
    return filePath ? path.resolve(this.testDir, filePath) : '';
  }

  private resolveLanguageTags(projectPath: string): Record<string, number> {
    const languageTags = this.homecheckArgs.languageTags ?? {};
    return Object.fromEntries(
      Object.entries(languageTags).map(([filePath, language]) => {
        return [path.normalize(path.resolve(projectPath, filePath)), language];
      })
    );
  }

  private resolveRuleOptions(): Record<string, object[]> | undefined {
    const ruleOptions = this.homecheckArgs.ruleOptions;
    if (!ruleOptions) {
      return undefined;
    }
    return JSON.parse(JSON.stringify(ruleOptions), (key: string, value: unknown) => {
      return key === 'path' && typeof value === 'string' ? path.resolve(this.testDir, value) : value;
    });
  }
}

function createHomeCheckLinterOptions(
  testModeProps: TestModeProperties,
  testCommonOpts?: LinterOptions,
  testModeArgs?: string,
  runTestFileOpts?: RunTestFileOptions
): LinterOptions {
  const linterOpts: LinterOptions = {};

  if (testCommonOpts) {
    Object.assign(linterOpts, testCommonOpts);
  }

  if (testModeArgs) {
    Object.assign(linterOpts, getLinterOptionsFromCommandLine(testModeArgs));
  }

  if (runTestFileOpts?.testRunnerOpts?.linterOptions) {
    Object.assign(linterOpts, runTestFileOpts.testRunnerOpts.linterOptions);
  }

  Object.assign(linterOpts, testModeProps.modeOpts);

  return linterOpts;
}

function getLinterOptionsFromCommandLine(cmdLine: string): LinterOptions {
  return parseCommandLine(getCommandLineArguments(cmdLine), { exitOnFail: false, disableErrorOutput: true }).
    linterOptions;
}
