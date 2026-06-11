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

import * as path from 'node:path';
import * as os from 'node:os';
import type { FileIssues } from 'homecheck';
import { transferIssues2ProblemInfo } from '../lib/HomeCheck';
import type { ProblemInfo } from '../lib/ProblemInfo';
import { ProblemSeverity } from '../lib/ProblemSeverity';

const HOMECHECK_NON_BLOCKING_SEVERITY = 3;

interface HomeCheckRunnerConfig {
  projectPath: string;
  testFile: string;
  rules?: string[];
  ruleSet?: string[];
  ruleOptions?: Record<string, object[]>;
  ohosSdkPath?: string;
  hmsSdkPath?: string;
  languageTags?: Record<string, number>;
  modeOpts?: { enableAutofix?: boolean; [key: string]: unknown };
}

interface HomeCheckRunnerResult {
  problems: ProblemInfo[];
}

interface HomeCheckTool {
  buildCheckEntry: () => Promise<boolean>;
  start: () => Promise<FileIssues[]>;
}

type MigrationToolConstructor = new (ruleConfigInfo: object, projectConfigInfo: object) => HomeCheckTool;

async function runHomeCheck(config: HomeCheckRunnerConfig): Promise<HomeCheckRunnerResult> {
  const homecheckRoot = path.resolve('homecheck');
  const MigrationTool = loadMigrationTool(homecheckRoot);
  const ruleConfigInfo = createRuleConfigInfo(config);
  const projectConfigInfo = createProjectConfigInfo(config, homecheckRoot);
  const migrationTool = new MigrationTool(ruleConfigInfo, projectConfigInfo);
  const built = await migrationTool.buildCheckEntry();
  if (!built) {
    throw new Error('Failed to build HomeCheck entry.');
  }
  const fileIssues = await migrationTool.start();
  const problemsByFile = transferIssues2ProblemInfo(fileIssues);
  const testFile = path.normalize(config.testFile);
  const problems = problemsByFile.get(testFile) ?? [];
  normalizeHomeCheckSeverity(problems);
  problems.sort(compareProblems);
  return { problems };
}

function loadMigrationTool(homecheckRoot: string): MigrationToolConstructor {
  // Use the workspace HomeCheck build so tests exercise local HomeCheck sources after `npm --prefix homecheck run compile`.
  // eslint-disable-next-line @typescript-eslint/no-require-imports
  const homecheck = require(path.join(homecheckRoot, 'lib/Index.js')) as { MigrationTool: MigrationToolConstructor };
  return homecheck.MigrationTool;
}

function createRuleConfigInfo(config: HomeCheckRunnerConfig): object {
  const rules = Object.fromEntries(
    (config.rules ?? []).map((rule) => {
      const ruleOptions = config.ruleOptions?.[rule] ?? [];
      return [rule, ruleOptions.length > 0 ? ['warn', ...ruleOptions] : 1];
    })
  );
  return {
    files: ['**/*.ets', '**/*.ts', '**/*.js'],
    ignore: [
      '**/ohosTest/**/*',
      '**/node_modules/**/*',
      '**/build/**/*',
      '**/hvigorfile/**/*',
      '**/oh_modules/**/*',
      '**/.preview/**/*'
    ],
    rules,
    ruleSet: config.ruleSet ?? [],
    overrides: [],
    extRuleSet: []
  };
}

function createProjectConfigInfo(config: HomeCheckRunnerConfig, homecheckRoot: string): object {
  return {
    projectName: 'HomeCheckTest',
    projectPath: config.projectPath,
    logPath: path.join(os.tmpdir(), `HomeCheckTest-${process.pid}.log`),
    ohosSdkPath: config.ohosSdkPath ?? '',
    hmsSdkPath: config.hmsSdkPath ?? '',
    checkPath: '',
    sdkVersion: 14,
    fix: config.modeOpts?.enableAutofix ? 'true' : 'false',
    npmPath: '',
    npmInstallDir: './',
    reportDir: '',
    arkCheckPath: homecheckRoot,
    product: 'default',
    languageTags: new Map(Object.entries(config.languageTags ?? {})),
    fileOrFolderToCheck: [config.testFile],
    logLevel: 'INFO',
    arkAnalyzerLogLevel: 'ERROR',
    sdksThirdParty: []
  };
}

function compareProblems(left: ProblemInfo, right: ProblemInfo): number {
  return left.line - right.line || left.column - right.column;
}

function normalizeHomeCheckSeverity(problems: ProblemInfo[]): void {
  problems.forEach((problem) => {
    if (problem.severity === HOMECHECK_NON_BLOCKING_SEVERITY) {
      problem.severity = ProblemSeverity.WARNING;
    }
  });
}

async function main(): Promise<void> {
  const encodedConfig = process.argv[2];
  if (!encodedConfig) {
    throw new Error('Missing HomeCheck runner config.');
  }
  const config = JSON.parse(Buffer.from(encodedConfig, 'base64').toString()) as HomeCheckRunnerConfig;
  const result = await runHomeCheck(config);
  process.stdout.write(JSON.stringify(result));
}

main().catch((error) => {
  process.stderr.write((error as Error).stack ?? (error as Error).message);
  process.exit(1);
});
