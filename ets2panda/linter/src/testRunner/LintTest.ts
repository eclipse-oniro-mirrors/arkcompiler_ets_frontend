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

import * as fs from 'node:fs';
import * as path from 'node:path';
import type { Autofix } from '../lib/autofixes/Autofixer';
import type { CommandLineOptions } from '../lib/CommandLineOptions';
import type { LinterConfig } from '../lib/LinterConfig';
import { lint } from '../lib/LinterRunner';
import type { LintRunResult } from '../lib/LintRunResult';
import { Logger } from '../lib/Logger';
import { TimeRecorder } from '../lib/statistics/scan/TimeRecorder';
import { compileLintOptions } from '../lib/ts-compiler/Compiler';
import { DIFF_EXT, RESULTS_DIR, TAB } from './Consts';
import type { CreateLintTestOptions } from './TestFactory';
import type { TestModeProperties } from './TestMode';
import { TestMode } from './TestMode';
import type { TestProblemInfo, TestResult } from './TestResult';
import { validateTestResult } from './TestResult';
import { transformProblemInfos } from './TestUtil';

export class LintTest {
  readonly testDir: string;
  readonly testFile: string;
  readonly testModeProps: TestModeProperties;
  readonly cmdOptions: CommandLineOptions;

  constructor(createLintTestOpts: CreateLintTestOptions) {
    this.testDir = createLintTestOpts.testDir;
    this.testFile = createLintTestOpts.testFile;
    this.testModeProps = createLintTestOpts.testModeProps;
    this.cmdOptions = createLintTestOpts.cmdOptions;
  }

  run(): boolean {
    Logger.info(`Running test ${this.testFile} (${TestMode[this.testModeProps.mode]} mode)`);

    const linterConfig = this.compile();
    const timeRecorder = new TimeRecorder();
    const linterResult = lint(linterConfig, timeRecorder);
    return this.validate(linterResult);
  }

  compile(): LinterConfig {
    return compileLintOptions(this.cmdOptions);
  }

  validate(actualLinterResult: LintRunResult): boolean {
    // Get actual test results.
    const fileProblems = actualLinterResult.problemsInfos.get(path.normalize(this.cmdOptions.inputFiles[0]));
    if (fileProblems === undefined) {
      return false;
    }
    const actualResult = transformProblemInfos(fileProblems, this.testModeProps.mode);

    // Read file with expected test result.
    const resultDiff = this.compareLintResult(actualResult);

    // Write file with actual test results.
    this.writeLintResultFile(actualResult, resultDiff);
    return !resultDiff;
  }

  private static readLintResultFile(testDir: string, testResultFileName: string): TestProblemInfo[] {
    const filePath = path.join(testDir, testResultFileName);
    try {
      const testResult = fs.readFileSync(filePath).toString();
      return validateTestResult(JSON.parse(testResult)).result;
    } catch (error) {
      throw new Error(`Failed to process ${filePath}: ${(error as Error).message}`);
    }
  }

  private compareLintResult(actual: TestProblemInfo[]): string {
    // Read file with expected test result.
    let diff: string = '';
    const testResultFileName = this.testFile + this.testModeProps.resultFileExt;
    try {
      let expected = LintTest.readLintResultFile(this.testDir, testResultFileName);

      /**
       * The exclusive field is added to identify whether the use case is exclusive to the RT or SDK
       * RT means the RT exclusive
       * SDK means the SDK exclusive
       * undefined means shared
       */
      expected = expected.filter((x) => {
        return !x?.exclusive || x.exclusive === (this.cmdOptions.linterOptions.useRtLogic ? 'RT' : 'SDK');
      });

      if (!expected || expected.length !== actual.length) {
        const expectedResultCount = expected ? expected.length : 0;
        diff = `Expected count: ${expectedResultCount} vs actual count: ${actual.length}`;
      } else {
        diff = LintTest.expectedAndActualMatch(expected, actual);
      }

      if (diff) {
        Logger.info(`${TAB}Lint test failed. Expected and actual results differ:\n${diff}`);
      }
    } catch (error) {
      // Write error message to diff, as non-empty diff indicates that test has failed.
      diff = (error as Error).message;
      Logger.info(`${TAB}Failed to compare expected and actual results. ` + diff);
    }

    return diff;
  }

  private static expectedAndActualMatch(expectedNodes: TestProblemInfo[], actualNodes: TestProblemInfo[]): string {
    // Compare expected and actual results.
    for (let i = 0; i < actualNodes.length; i++) {
      const actual = actualNodes[i];
      const expected = expectedNodes[i];
      if (!LintTest.locationMatch(expected, actual) || actual.problem !== expected.problem) {
        return LintTest.reportLintResultDiff(expected, actual);
      }
      if (!LintTest.autofixArraysMatch(expected.autofix, actual.autofix)) {
        return LintTest.reportLintResultDiff(expected, actual);
      }
      if (expected.suggest && actual.suggest !== expected.suggest) {
        return LintTest.reportLintResultDiff(expected, actual);
      }
      if (expected.rule && actual.rule !== expected.rule) {
        return LintTest.reportLintResultDiff(expected, actual);
      }
      if (expected.severity && actual.severity !== expected.severity) {
        return LintTest.reportLintResultDiff(expected, actual);
      }
    }

    return '';
  }

  private static locationMatch(expected: TestProblemInfo, actual: TestProblemInfo): boolean {
    return (
      actual.line === expected.line &&
      actual.column === expected.column &&
      (!expected.endLine || actual.endLine === expected.endLine) &&
      (!expected.endColumn || actual.endColumn === expected.endColumn)
    );
  }

  private static autofixArraysMatch(expected: Autofix[] | undefined, actual: Autofix[] | undefined): boolean {
    if (!expected && !actual) {
      return true;
    }
    if (!(expected && actual) || expected.length !== actual.length) {
      return false;
    }
    for (let i = 0; i < actual.length; ++i) {
      if (
        actual[i].start !== expected[i].start ||
        actual[i].end !== expected[i].end ||
        actual[i].replacementText !== expected[i].replacementText
      ) {
        return false;
      }
    }
    return true;
  }

  private writeLintResultFile(actual: TestProblemInfo[], diff: string): void {
    const actualResultsDir = path.join(this.testDir, RESULTS_DIR);
    if (!fs.existsSync(actualResultsDir)) {
      fs.mkdirSync(actualResultsDir);
    }

    const actualTestResult: TestResult = { result: actual };
    const actualResultJSON = JSON.stringify(actualTestResult, null, 4);
    fs.writeFileSync(path.join(actualResultsDir, this.testFile + this.testModeProps.resultFileExt), actualResultJSON);

    if (diff) {
      fs.writeFileSync(path.join(actualResultsDir, this.testFile + this.testModeProps.resultFileExt + DIFF_EXT), diff);
    }
  }

  private static reportLintResultDiff(expected: TestProblemInfo, actual: TestProblemInfo): string {
    const expectedNode = JSON.stringify({ nodes: [expected] }, null, 4);
    const actualNode = JSON.stringify({ nodes: [actual] }, null, 4);

    const diff = `Expected:
${expectedNode}
Actual:
${actualNode}`;

    return diff;
  }
}
