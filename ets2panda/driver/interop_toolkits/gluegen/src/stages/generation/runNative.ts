/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { promises as fs } from 'node:fs';
import * as path from 'node:path';

import {
  GlueGenDiagnosticError,
  GlueGenErrorCode,
  GlueGenErrorList,
  GlueGenInternalError,
  errorMessage,
} from '../../errors';
import type { GlueGenReport } from '../../contracts';
import { LogData, type ILogger } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import { NativeExitCode, NativeProcessError, type NativeProcessResult } from '../../native';
import { GlueGenInvocation } from '../../native/invocation';
import { parseGlueGenReport } from '../../native/report';
import { ARKTS_CONFIG_ARTIFACT, CONFIGURATION_ARTIFACT, INTEROP_FILE_LIST_ARTIFACT } from '../stageArtifacts';

const DEFAULT_NATIVE_TIMEOUT_MS = 5 * 60 * 1000;
const DEFAULT_MAX_OUTPUT_BYTES = 1024 * 1024;

type GenerationRequirements = readonly [
  typeof CONFIGURATION_ARTIFACT,
  typeof INTEROP_FILE_LIST_ARTIFACT,
  typeof ARKTS_CONFIG_ARTIFACT,
];

/** Generation hook: invokes native gluegen and reads its on-disk report. */
export async function runNative(scope: StageScope<GlueGenContext, GenerationRequirements>): Promise<void> {
  const { context } = scope;
  const { moduleTable } = scope.get(CONFIGURATION_ARTIFACT);
  const inputFileListPath = scope.get(INTEROP_FILE_LIST_ARTIFACT);
  if (!(await hasInputFiles(inputFileListPath))) {
    return;
  }
  const arktsConfigPath = scope.get(ARKTS_CONFIG_ARTIFACT);
  const workspaceRoot = path.join(moduleTable.mainModule.cachePath, 'gluegen');
  const intermediatesPath = path.join(workspaceRoot, 'intermediates');
  const outputPath = context.buildConfig.declgenBridgeConfigPath;
  const reportPath = path.join(workspaceRoot, 'report.json');
  const invocation = GlueGenInvocation.builder({
    executable: context.runtime.nativeExecutablePath,
    cwd: moduleTable.mainModule.projectRootPath,
    environment: process.env,
    signal: undefined,
    timeoutMs: DEFAULT_NATIVE_TIMEOUT_MS,
    maxOutputBytes: DEFAULT_MAX_OUTPUT_BYTES,
  })
    .inputFileList(inputFileListPath)
    .arktsConfig(arktsConfigPath)
    .output(outputPath)
    .cachePath(workspaceRoot)
    .reportPath(reportPath)
    .build();

  let processResult: NativeProcessResult;
  try {
    await fs.mkdir(path.dirname(outputPath), { recursive: true });
    await fs.mkdir(intermediatesPath, { recursive: true });
    await fs.rm(reportPath, { force: true });
    processResult = await invocation.execute();
  } catch (error) {
    const data = new LogData({
      code: GlueGenErrorCode.NATIVE_PROCESS_FAIL,
      description: 'Gluegen could not complete native process execution.',
      cause: errorMessage(error, 'unknown native generation failure'),
      ...(error instanceof NativeProcessError ? { moreInfo: { processFailureKind: error.kind } } : {}),
    });
    throw new GlueGenInternalError(data);
  }
  const report = await readNativeReport(reportPath, processResult);
  printNativeDiagnostics(context.logger, report, processResult.exitCode);
  const exitCode = requireSupportedNativeExitCode(processResult);
  classifyNativeResult(processResult, exitCode, report.diagnostics.errors);
  await requireOutputFile(outputPath);
}

async function hasInputFiles(inputFileListPath: string): Promise<boolean> {
  const inputFileList = await fs.readFile(inputFileListPath, 'utf8');
  return inputFileList.trim().length > 0;
}

function classifyNativeResult(
  processResult: NativeProcessResult,
  exitCode: NativeExitCode,
  diagnosticErrors: readonly LogData[],
): void {
  if (exitCode === NativeExitCode.InternalError) {
    throwNativeInternalFailure('native gluegen reported an internal failure', processResult);
  }
  if (exitCode === NativeExitCode.DiagnosticError) {
    throwDiagnosticResult(diagnosticErrors);
  }
}

function requireSupportedNativeExitCode(processResult: NativeProcessResult): NativeExitCode {
  if (processResult.signal !== null) {
    throwNativeInternalFailure(`native process terminated by signal ${processResult.signal}`, processResult);
  }
  if (processResult.exitCode === null) {
    throwNativeInternalFailure('native process finished without an exit code', processResult);
  }
  if (
    processResult.exitCode !== NativeExitCode.Success &&
    processResult.exitCode !== NativeExitCode.DiagnosticError &&
    processResult.exitCode !== NativeExitCode.InternalError
  ) {
    throwNativeInternalFailure(
      `native gluegen returned unsupported exit code ${processResult.exitCode}`,
      processResult,
    );
  }
  return processResult.exitCode;
}

function throwDiagnosticResult(diagnosticErrors: readonly LogData[]): never {
  throw new GlueGenErrorList(diagnosticErrors.map((data) => new GlueGenDiagnosticError(data)));
}

async function readNativeReport(reportPath: string, processResult: NativeProcessResult): Promise<GlueGenReport> {
  let report: GlueGenReport;
  try {
    const serialized = await fs.readFile(reportPath, 'utf8');
    report = parseGlueGenReport(serialized);
  } catch (error) {
    throwNativeInternalFailure('native gluegen did not produce a valid report JSON file', processResult, error, {
      reportPath,
    });
  }
  return report;
}

async function requireOutputFile(outputPath: string): Promise<void> {
  try {
    const stat = await fs.stat(outputPath);
    if (!stat.isFile()) {
      throw new Error('output path is not a file');
    }
  } catch (error) {
    const data = new LogData({
      code: GlueGenErrorCode.NATIVE_RESPONSE_FAIL,
      description: 'Native gluegen did not produce its output JSON file.',
      cause: errorMessage(error, 'unknown native generation failure'),
      position: outputPath,
    });
    throw new GlueGenInternalError(data);
  }
}

function printNativeDiagnostics(logger: ILogger, report: GlueGenReport, exitCode: number | null): void {
  report.diagnostics.warnings.forEach((warning) => logger.printWarn(warning.description));
  if (exitCode !== NativeExitCode.DiagnosticError) {
    report.diagnostics.errors.forEach((error) => logger.printError(error));
  }
}

function throwNativeInternalFailure(
  description: string,
  processResult: NativeProcessResult,
  responseError?: unknown,
  details: Readonly<Record<string, unknown>> = {},
): never {
  const cause = responseError === undefined ? '' : errorMessage(responseError, 'unknown native generation failure');
  const data = new LogData({
    code: GlueGenErrorCode.NATIVE_RESPONSE_FAIL,
    description,
    cause,
    moreInfo: {
      ...details,
      exitCode: processResult.exitCode,
      signal: processResult.signal,
      stderrBytes: Buffer.byteLength(processResult.stderr),
    },
  });
  throw new GlueGenInternalError(data);
}
