/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
import type { FileIssues, RuleFix } from 'homecheck';
import type { CommandLineOptions } from './CommandLineOptions';
import type { ProblemInfo } from './ProblemInfo';
import { FaultID } from './Problems';
import { shouldProcessFile } from './LinterRunner';

interface RuleConfigInfo {
  ruleSet: string[];
}

interface ProjectConfigInfo {
  projectName: string | undefined;
  projectPath: string | undefined;
  logPath: string;
  arkCheckPath: string;
  ohosSdkPath: string;
  hmsSdkPath: string;
  reportDir: string;
  languageTags: Map<string, number>;
  fileOrFolderToCheck: string[];
}

export function getHomeCheckConfigInfo(cmdOptions: CommandLineOptions): {
  ruleConfigInfo: RuleConfigInfo;
  projectConfigInfo: ProjectConfigInfo;
} {
  let inputFiles = cmdOptions.inputFiles;
  let fliesTocheck: string[] = inputFiles;
  if (cmdOptions.scanWholeProjectInHomecheck === true) {
    fliesTocheck = [];
  }
  inputFiles = inputFiles.filter((input) => {
    return shouldProcessFile(cmdOptions, input);
  });
  const languageTags = new Map<string, number>();
  inputFiles.forEach((file) => {
    languageTags.set(path.normalize(file), 2);
  });
  const ruleConfigInfo = {
    ruleSet: ['plugin:@migration/all'],
    files: ['**/*.ets', '**/*.ts', '**/*.js']
  };
  const projectConfigInfo = {
    projectName: cmdOptions.arktsWholeProjectPath,
    projectPath: cmdOptions.arktsWholeProjectPath,
    logPath: cmdOptions.outputFilePath ? path.join(cmdOptions.outputFilePath, 'HomeCheck.log') : './HomeCheck.log',
    arkCheckPath: './node_modules/homecheck',
    ohosSdkPath: cmdOptions.sdkDefaultApiPath ? cmdOptions.sdkDefaultApiPath : '',
    hmsSdkPath: cmdOptions.sdkExternalApiPath ? cmdOptions.sdkExternalApiPath[0] : '',
    reportDir: './',
    languageTags: languageTags,
    fileOrFolderToCheck: fliesTocheck,
    logLevel: cmdOptions.verbose ? 'DEBUG' : 'INFO',
    arkAnalyzerLogLevel: cmdOptions.verbose ? 'DEBUG' : 'ERROR'
  };
  return { ruleConfigInfo, projectConfigInfo };
}

export function transferIssues2ProblemInfo(fileIssuesArray: FileIssues[]): Map<string, ProblemInfo[]> {
  const result = new Map<string, ProblemInfo[]>();
  fileIssuesArray.forEach((fileIssues) => {
    fileIssues.issues.forEach((issueReport) => {
      const defect = issueReport.defect;
      const problemInfo: ProblemInfo = {
        line: defect.reportLine,
        column: defect.reportColumn,
        endLine: defect.reportLine,
        endColumn: defect.reportColumn,
        start: 0,
        end: 0,
        type: '',
        severity: defect.severity,
        faultId: FaultID.LAST_ID,
        problem: defect.problem,
        suggest: '',
        rule: defect.description,
        ruleTag: -1,
        autofixable: defect.fixable
      };
      if (problemInfo.autofixable) {
        const fix = issueReport.fix as RuleFix;
        const replacementText = fix.text;
        const start = fix.range[0];
        const end = fix.range[1];
        problemInfo.autofix = [{ replacementText, start, end }];
        problemInfo.autofixTitle = defect.ruleId;
      }
      const filePath = path.normalize(defect.mergeKey.split('%')[0]);
      const problems = result.get(filePath) || [];
      problems.push(problemInfo);
      result.set(filePath, problems);
    });
  });
  return result;
}

export function removeOutOfRangeFiles(
  homeCheckResult: Map<string, ProblemInfo[]>,
  cmdOptions: CommandLineOptions
): Map<string, ProblemInfo[]> {
  const filteredResult = new Map<string, ProblemInfo[]>();

  for (const [filePath, problems] of homeCheckResult) {
    if (cmdOptions.inputFiles.includes(filePath)) {
      filteredResult.set(filePath, problems);
    }
  }

  return filteredResult;
}
