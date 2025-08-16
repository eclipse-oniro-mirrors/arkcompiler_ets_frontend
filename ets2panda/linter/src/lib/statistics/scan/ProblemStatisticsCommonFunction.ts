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
import type { LintRunResult } from '../../LintRunResult';
import { Logger } from '../../Logger';
import type { ProblemInfo } from '../../ProblemInfo';
import * as mk from '../../utils/consts/MapKeyConst';
import { countFiles } from './CountFile';
import { countNapiFiles } from './CountNapiFile';
import type { ProblemNumbersInfo } from './ProblemNumbersInfo';
import type { ProblemStatisticsInfo } from './ProblemStatisticsInfo';
import type { RuleDetailedErrorInfo } from './RuleDetailedErrorInfo';
import type { StatisticsReportInPutInfo } from './StatisticsReportInPutInfo';
import type { TimeRecorder } from './TimeRecorder';
import { WorkLoadInfo } from './WorkLoadInfo';

export function getProblemStatisticsInfo(
  problemNumbers: ProblemNumbersInfo,
  ruleToNumbersMap: Map<string, number>,
  ruleToAutoFixedNumbersMap: Map<string, number>,
  timeRecorder: TimeRecorder,
  WorkLoadInfo?: WorkLoadInfo
): ProblemStatisticsInfo {
  const problemNumberMap: Map<string, number> = new Map();
  problemNumberMap.set(mk.TOTAL_PROBLEMS, problemNumbers.totalProblemNumbers);
  problemNumberMap.set(mk.ONE_POINT_ONE_PROBLEMS, problemNumbers.arkOnePointOneProblemNumbers);
  problemNumberMap.set(mk.ONE_POINT_TWO_PROBLEMS, problemNumbers.arkOnePointTWOProblemNumbers);
  problemNumberMap.set(mk.CAN_BE_AUTO_FIXED_PROBLEMS_NUMBERS, problemNumbers.canBeAutoFixedproblemNumbers);
  problemNumberMap.set(mk.NEED_TO_NAMUAL_FIX_PROBLEM_NUMBERS, problemNumbers.needToManualFixproblemNumbers);

  const scanTime = timeRecorder.getScanTime();
  const migrationTime = timeRecorder.getMigrationTime();
  const usedTimeMap: Map<string, string> = new Map();
  usedTimeMap.set(mk.SCAN_TIME, scanTime);
  usedTimeMap.set(mk.MIGRATION_TIME, migrationTime);

  const detailRuleProblemsMap: Map<string, RuleDetailedErrorInfo> = new Map();
  ruleToNumbersMap.forEach((value, key) => {
    const ruleDetailedErrorInfo: RuleDetailedErrorInfo = {
      rule: key,
      problemNumbers: value,
      canBeAutoFixedMumbers: ruleToAutoFixedNumbersMap.get(key) ?? 0
    };
    detailRuleProblemsMap.set(key, ruleDetailedErrorInfo);
  });

  const problemStatisticsInfo: ProblemStatisticsInfo = {
    problems: Object.fromEntries(problemNumberMap),
    usedTime: Object.fromEntries(usedTimeMap),
    WorkLoadInfo: WorkLoadInfo,
    eachRuleProblemsDetail: Array.from(detailRuleProblemsMap.values())
  };
  return problemStatisticsInfo;
}

export function getArktsOnePointOneProlemNumbers(problems: ProblemInfo[]): number {
  let problemNumbersForATSOnePointOne: number = 0;
  const signForOne: string = 's2d';
  problems.forEach((problem) => {
    if (problem.rule !== undefined) {
      if (problem.rule.includes(signForOne)) {
        problemNumbersForATSOnePointOne = problemNumbersForATSOnePointOne + 1;
      }
    }
  });
  return problemNumbersForATSOnePointOne;
}

export function accumulateRuleNumbers(
  problems: ProblemInfo[],
  ruleToNumbersMap: Map<string, number>,
  ruleToAutoFixedNumbersMap: Map<string, number>
): void {
  problems.forEach((problem) => {
    if (problem.rule !== undefined) {
      if (problem.autofix) {
        const currentNumber = ruleToAutoFixedNumbersMap.get(problem.rule) || 0;
        ruleToAutoFixedNumbersMap.set(problem.rule, currentNumber + 1);
      }
      const currentNumber = ruleToNumbersMap.get(problem.rule) || 0;
      ruleToNumbersMap.set(problem.rule, currentNumber + 1);
    }
  });
}

export async function generateReportFile(reportName: string, reportData, reportPath?: string): Promise<void> {
  let reportFilePath = path.join(reportName);
  if (reportPath !== undefined) {
    reportFilePath = path.join(path.normalize(reportPath), reportName);
  }
  try {
    await fs.promises.mkdir(path.dirname(reportFilePath), { recursive: true });
    await fs.promises.writeFile(reportFilePath, JSON.stringify(reportData, null, 2));
  } catch (error) {
    Logger.error(`Error generating report file:${error}`);
  }
}

export function generateReportFileSync(reportName: string, reportData: any, reportPath?: string): void {
  let reportFilePath = reportName;
  if (reportPath !== undefined) {
    reportFilePath = path.join(path.normalize(reportPath), reportName);
  }
  try {
    fs.mkdirSync(path.dirname(reportFilePath), { recursive: true });
    fs.writeFileSync(reportFilePath, JSON.stringify(reportData, null, 2));
  } catch (error) {
    Logger.error(`Error generating report file:${error}`);
  }
}

export function getMapValueSum(map: Map<any, number>): number {
  let result = 0;
  for (const value of map.values()) {
    result += value;
  }
  return result;
}

export async function generateScanProbelemStatisticsReport(
  statisticsReportInPutInfo: StatisticsReportInPutInfo
): Promise<void> {
  const canBeAutoFixedproblemNumbers = getMapValueSum(statisticsReportInPutInfo.ruleToAutoFixedNumbersMap);
  const problemNumbers: ProblemNumbersInfo = {
    totalProblemNumbers: statisticsReportInPutInfo.totalProblemNumbers,
    arkOnePointOneProblemNumbers: statisticsReportInPutInfo.arkOnePointOneProblemNumbers,
    arkOnePointTWOProblemNumbers:
      statisticsReportInPutInfo.totalProblemNumbers - statisticsReportInPutInfo.arkOnePointOneProblemNumbers,
    canBeAutoFixedproblemNumbers: canBeAutoFixedproblemNumbers,
    needToManualFixproblemNumbers: statisticsReportInPutInfo.totalProblemNumbers - canBeAutoFixedproblemNumbers
  };
  const projectFolderList = statisticsReportInPutInfo.cmdOptions.linterOptions.projectFolderList!;
  const workLoadInfo = await getWorkLoadInfo(projectFolderList);
  workLoadInfo.calculateFixRate(problemNumbers);
  const statisticsReportData = getProblemStatisticsInfo(
    problemNumbers,
    statisticsReportInPutInfo.ruleToNumbersMap,
    statisticsReportInPutInfo.ruleToAutoFixedNumbersMap,
    statisticsReportInPutInfo.timeRecorder,
    workLoadInfo
  );
  await generateReportFile(
    statisticsReportInPutInfo.statisticsReportName,
    statisticsReportData,
    statisticsReportInPutInfo.cmdOptions.outputFilePath
  );
}

export function generateMigrationStatisicsReport(
  lintResult: LintRunResult,
  timeRecorder: TimeRecorder,
  outputFilePath?: string
): void {
  const ruleToNumbersMap: Map<string, number> = new Map();
  const ruleToAutoFixedNumbersMap: Map<string, number> = new Map();
  let arkOnePointOneProblemNumbers: number = 0;

  const problemsInfoAfterAutofixed = lintResult.problemsInfos;
  for (const problems of problemsInfoAfterAutofixed.values()) {
    accumulateRuleNumbers(problems, ruleToNumbersMap, ruleToAutoFixedNumbersMap);
    arkOnePointOneProblemNumbers = arkOnePointOneProblemNumbers + getArktsOnePointOneProlemNumbers(problems);
  }

  let totalProblemNumbers: number = 0;
  for (const problems of problemsInfoAfterAutofixed.values()) {
    totalProblemNumbers += problems.length;
  }

  const canBeAutoFixedproblemNumbers = getMapValueSum(ruleToAutoFixedNumbersMap);

  const problemNumbers: ProblemNumbersInfo = {
    totalProblemNumbers: totalProblemNumbers,
    arkOnePointOneProblemNumbers: arkOnePointOneProblemNumbers,
    arkOnePointTWOProblemNumbers: totalProblemNumbers - arkOnePointOneProblemNumbers,
    canBeAutoFixedproblemNumbers: canBeAutoFixedproblemNumbers,
    needToManualFixproblemNumbers: totalProblemNumbers - canBeAutoFixedproblemNumbers
  };

  const statisticsReportData = getProblemStatisticsInfo(
    problemNumbers,
    ruleToNumbersMap,
    ruleToAutoFixedNumbersMap,
    timeRecorder
  );
  const statisticsReportName: string = 'migration-results-statistics.json';
  generateReportFileSync(statisticsReportName, statisticsReportData, outputFilePath);
}

export async function getWorkLoadInfo(projectPathList: string[]): Promise<WorkLoadInfo> {
  const workloadInfo = new WorkLoadInfo();
  const projectResults = await Promise.all(
    projectPathList.map(async(projectPath) => {
      const normalizedPath = path.normalize(projectPath);
      const [countFilesResults, countNapiFileResults] = await Promise.all([
        countFiles(normalizedPath),
        countNapiFiles(normalizedPath)
      ]);

      const getLines = (lang: keyof typeof countFilesResults): number => {
        return countFilesResults[lang]?.lineCount || 0;
      };

      return {
        normalizedPath,
        arkTSCodeLines: getLines('ArkTS') + getLines('ArkTS Test'),
        cAndCPPCodeLines: getLines('C/C++'),
        napiCodeLines: countNapiFileResults.napiLines,
        jsCodeLines: getLines('JavaScript'),
        tsCodeLines: getLines('TypeScript'),
        jsonCodeLines: getLines('JSON'),
        xmlCodeLines: getLines('XML')
      };
    })
  );

  projectResults.forEach((result) => {
    workloadInfo.addFloderResult(result);
  });
  return workloadInfo;
}
