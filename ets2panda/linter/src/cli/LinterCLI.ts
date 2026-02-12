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

import { MigrationTool } from 'homecheck';
import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import * as readlineSync from 'readline-sync';
import * as readline from 'node:readline';
import type { CommandLineOptions } from '../lib/CommandLineOptions';
import { getHomeCheckConfigInfo, transferIssues2ProblemInfo, removeOutOfRangeFiles } from '../lib/HomeCheck';
import { lint } from '../lib/LinterRunner';
import { Logger } from '../lib/Logger';
import type { ProblemInfo } from '../lib/ProblemInfo';
import * as statistic from '../lib/statistics/scan/ProblemStatisticsCommonFunction';
import type { ScanTaskRelatedInfo } from '../lib/statistics/scan/ScanTaskRelatedInfo';
import { StatisticsReportInPutInfo } from '../lib/statistics/scan/StatisticsReportInPutInfo';
import { TimeRecorder } from '../lib/statistics/scan/TimeRecorder';
import { logStatistics } from '../lib/statistics/StatisticsLogger';
import { compileLintOptions } from '../lib/ts-compiler/Compiler';
import { processSyncErr, processSyncOut } from '../lib/utils/functions/ProcessWrite';
import { parseCommandLine } from './CommandLineParser';
import { getAllLinterRules } from '../lib/utils/functions/ConfiguredRulesProcess';
import { tryCallGC } from '../lib/utils/functions/GarbageCollectorUtils';
import { mergeArrayMaps } from '../lib/utils/functions/MergeArrayMaps';

export function run(): void {
  const commandLineArgs = process.argv.slice(2);
  if (commandLineArgs.length === 0) {
    Logger.info('Command line error: no arguments');
    process.exit(-1);
  }

  const cmdOptions = parseCommandLine(commandLineArgs);
  if (cmdOptions.linterOptions.migratorMode && cmdOptions.linterOptions.autofixCheck) {
    const shouldRun = readlineSync.question('Do you want to run the linter and apply migration? (y/n): ').toLowerCase();
    if (shouldRun !== 'y') {
      console.log('Linting canceled by user.');
      process.exit(0);
    }
  }

  if (cmdOptions.devecoPluginModeDeprecated) {
    runIdeModeDeprecated(cmdOptions);
  } else if (cmdOptions.linterOptions.ideInteractive) {
    runIdeInteractiveMode(cmdOptions);
  } else {
    const compileOptions = compileLintOptions(cmdOptions);
    const result = lint(compileOptions, new TimeRecorder());
    logStatistics(result.projectStats);
    process.exit(result.hasErrors ? 1 : 0);
  }
}

async function runIdeInteractiveMode(cmdOptions: CommandLineOptions): Promise<void> {
  cmdOptions.followSdkSettings = true;
  cmdOptions.disableStrictDiagnostics = true;
  const timeRecorder = new TimeRecorder();
  const scanTaskRelatedInfo = {} as ScanTaskRelatedInfo;
  scanTaskRelatedInfo.cmdOptions = cmdOptions;
  scanTaskRelatedInfo.timeRecorder = timeRecorder;
  await executeScanTask(scanTaskRelatedInfo);

  const statisticsReportInPutInfo = scanTaskRelatedInfo.statisticsReportInPutInfo;
  statisticsReportInPutInfo.statisticsReportName = 'scan-problems-statistics.json';
  statisticsReportInPutInfo.totalProblemNumbers = getTotalProblemNumbers(scanTaskRelatedInfo.mergedProblems);
  statisticsReportInPutInfo.cmdOptions = cmdOptions;
  statisticsReportInPutInfo.timeRecorder = timeRecorder;
  statisticsReportInPutInfo.allLinterRules = getAllLinterRules();

  if (!cmdOptions.linterOptions.migratorMode && statisticsReportInPutInfo.cmdOptions.linterOptions.projectFolderList) {
    await statistic.generateScanProbelemStatisticsReport(statisticsReportInPutInfo);
  }

  const mergedProblems = scanTaskRelatedInfo.mergedProblems;
  const reportData = Object.fromEntries(mergedProblems);
  const reportName: string = 'scan-report.json';
  await statistic.generateReportFile(reportName, reportData, cmdOptions.outputFilePath);
  for (const [filePath, problems] of mergedProblems) {
    const reportLine = JSON.stringify({ filePath, problems }) + '\n';
    await processSyncOut(reportLine);
  }
  await processSyncErr('{"content":"report finish","messageType":1,"indictor":1}\n');
  process.exit(0);
}

function getTotalProblemNumbers(mergedProblems: Map<string, ProblemInfo[]>): number {
  let totalProblemNumbers: number = 0;
  for (const problems of mergedProblems.values()) {
    totalProblemNumbers += problems.length;
  }
  return totalProblemNumbers;
}

async function executeScanTask(scanTaskRelatedInfo: ScanTaskRelatedInfo): Promise<void> {
  const cmdOptions = scanTaskRelatedInfo.cmdOptions;
  scanTaskRelatedInfo.statisticsReportInPutInfo = new StatisticsReportInPutInfo();
  scanTaskRelatedInfo.statisticsReportInPutInfo.ruleToNumbersMap = new Map();
  scanTaskRelatedInfo.statisticsReportInPutInfo.ruleToAutoFixedNumbersMap = new Map();
  scanTaskRelatedInfo.mergedProblems = new Map<string, ProblemInfo[]>();
  if (cmdOptions.linterOptions.arkts2 && cmdOptions.homecheck) {
    await executeHomeCheckTask(scanTaskRelatedInfo);
    tryCallGC();
  }

  if (!cmdOptions.skipLinter) {
    const compileOptions = compileLintOptions(cmdOptions);
    scanTaskRelatedInfo.compileOptions = compileOptions;
    executeLintTask(scanTaskRelatedInfo);
  }
}

async function executeHomeCheckTask(scanTaskRelatedInfo: ScanTaskRelatedInfo): Promise<void> {
  const cmdOptions = scanTaskRelatedInfo.cmdOptions;
  const { ruleConfigInfo, projectConfigInfo } = getHomeCheckConfigInfo(cmdOptions);
  let migrationTool: MigrationTool | null = new MigrationTool(ruleConfigInfo, projectConfigInfo);
  await migrationTool.buildCheckEntry();
  tryCallGC();
  scanTaskRelatedInfo.timeRecorder.startScan();
  scanTaskRelatedInfo.timeRecorder.setHomeCheckCountStatus(true);
  const result = await migrationTool.start();
  migrationTool = null;
  scanTaskRelatedInfo.homeCheckResult = transferIssues2ProblemInfo(result);
  scanTaskRelatedInfo.homeCheckResult = removeOutOfRangeFiles(
    scanTaskRelatedInfo.homeCheckResult,
    scanTaskRelatedInfo.cmdOptions
  );
}

function executeLintTask(scanTaskRelatedInfo: ScanTaskRelatedInfo): void {
  const cmdOptions = scanTaskRelatedInfo.cmdOptions;
  const compileOptions = scanTaskRelatedInfo.compileOptions;
  const homeCheckResult = scanTaskRelatedInfo.homeCheckResult;
  if (!scanTaskRelatedInfo.timeRecorder.getHomeCheckCountStatus()) {
    scanTaskRelatedInfo.timeRecorder.startScan();
  }
  const result = lint(compileOptions, scanTaskRelatedInfo.timeRecorder, homeCheckResult);
  // if migratorMode, lint result is merged result
  scanTaskRelatedInfo.mergedProblems = cmdOptions.linterOptions.migratorMode ?
    result.problemsInfos :
    mergeArrayMaps(homeCheckResult, result.problemsInfos);
  filterLintProblems(scanTaskRelatedInfo.mergedProblems, cmdOptions);
  accumulateRuleNumbersOfMergeProblems(scanTaskRelatedInfo);
}

function accumulateRuleNumbersOfMergeProblems(scanTaskRelatedInfo: ScanTaskRelatedInfo): void {
  for (const [, problems] of scanTaskRelatedInfo.mergedProblems) {
    statistic.accumulateRuleNumbers(
      problems,
      scanTaskRelatedInfo.statisticsReportInPutInfo.ruleToNumbersMap,
      scanTaskRelatedInfo.statisticsReportInPutInfo.ruleToAutoFixedNumbersMap
    );
    scanTaskRelatedInfo.statisticsReportInPutInfo.arkOnePointOneProblemNumbers +=
      statistic.getArktsOnePointOneProlemNumbers(problems);
  }
}

function filterLintProblems(mergedProblems: Map<string, ProblemInfo[]>, cmdOptions: CommandLineOptions): void {
  let filteredProblems: ProblemInfo[] = [];

  for (const file of mergedProblems.keys()) {
    const totalProblems = mergedProblems.get(file);

    if (totalProblems === undefined) {
      continue;
    }
    if (cmdOptions.scanWholeProjectInHomecheck) {
      if (cmdOptions.inputFiles.includes(file)) {
        continue;
      }
      filteredProblems = totalProblems.filter((problem) => {
        return problem.rule.includes('s2d');
      });
      if (filteredProblems.length > 0) {
        mergedProblems.set(file, filteredProblems);
      } else {
        mergedProblems.delete(file);
      }
      continue;
    }

    filteredProblems = totalProblems.filter((problem) => {
      return (
        !problem.rule.includes('s2d') &&
        !problem.rule.includes('d2s') &&
        !problem.rule.includes('js2s') &&
        !problem.rule.includes('ts2s')
      );
    });
    if (filteredProblems.length > 0) {
      mergedProblems.set(file, filteredProblems);
    } else {
      mergedProblems.delete(file);
    }
  }
}

function getTempFileName(): string {
  return path.join(os.tmpdir(), Math.floor(Math.random() * 10000000).toString() + '_linter_tmp_file.ts');
}

function showJSONMessage(problems: ProblemInfo[][]): void {
  const jsonMessage = problems[0].map((x) => {
    return {
      line: x.line,
      column: x.column,
      start: x.start,
      end: x.end,
      type: x.type,
      suggest: x.suggest,
      rule: x.rule,
      severity: x.severity,
      autofix: x.autofix
    };
  });
  Logger.info(`{"linter messages":${JSON.stringify(jsonMessage)}}`);
}

function runIdeModeDeprecated(cmdOptions: CommandLineOptions): void {
  const tmpFileName = getTempFileName();
  // read data from stdin
  const writeStream = fs.createWriteStream(tmpFileName, { flags: 'w' });
  const rl = readline.createInterface({
    input: process.stdin,
    output: writeStream,
    terminal: false
  });

  rl.on('line', (line: string) => {
    fs.appendFileSync(tmpFileName, line + '\n');
  });
  rl.once('close', () => {
    // end of input
    writeStream.close();
    cmdOptions.inputFiles = [tmpFileName];
    if (cmdOptions.parsedConfigFile) {
      cmdOptions.parsedConfigFile.fileNames.push(tmpFileName);
    }
    const compileOptions = compileLintOptions(cmdOptions);
    const result = lint(compileOptions, new TimeRecorder());
    const problems = Array.from(result.problemsInfos.values());
    if (problems.length === 1) {
      showJSONMessage(problems);
    } else {
      Logger.error('Unexpected error: could not lint file');
    }
    fs.unlinkSync(tmpFileName);
  });
}
