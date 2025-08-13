/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { ErrorCode } from "../error_code";
import { getEs2pandaPath } from "../init/process_build_config";
import { LogData, LogDataFactory, Logger } from "../logger";
import { CompilePayload } from "../types";
import { Task, WorkerInfo } from "./TaskManager";

export function handleCompileWorkerExit(
  workerInfo: WorkerInfo,
  code: number | null,
  signal: NodeJS.Signals | null,
  runningTasks: Map<string, Task<CompilePayload>>
): void {
  if (!code || code === 0) {
    return
  }
  const taskId = workerInfo.currentTaskId;
  const payload = runningTasks.get(taskId!)?.payload;
  if (!payload) {
    return;
  }
  const es2pandPath = getEs2pandaPath(payload.buildConfig);
  const cmd = [
    es2pandPath,
    '--arktsconfig', payload.fileInfo.arktsConfigFile,
    '--output', payload.fileInfo.abcFilePath,
    payload.fileInfo.filePath
  ];

  const logData: LogData = LogDataFactory.newInstance(
    ErrorCode.BUILDSYSTEM_COMPILE_FAILED_IN_WORKER,
    `Compile file ${payload.fileInfo.filePath} crashed (exit code ${code})`,
    "",
    "",
    [`Please try to run command locally : ${cmd.join(' ')}`]
  );

  Logger.getInstance().printErrorAndExit(logData);
}

export function handleDeclgenWorkerExit(
  workerInfo: WorkerInfo,
  code: number | null,
  signal: NodeJS.Signals | null,
  runningTasks: Map<string, Task<CompilePayload>>
): void {

  if (code && code !== 0) {
    let logExitCodeData: LogData = LogDataFactory.newInstance(
      ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER,
      `Declgen crashed (exit code ${code})`,
      "This error is likely caused internally from compiler.",
    );
    Logger.getInstance().printError(logExitCodeData);
    return;
  }
  if (signal && signal !== "SIGTERM") {
    let logSignalData: LogData = LogDataFactory.newInstance(
      ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER,
      `Declgen crashed (exit signal ${signal})`,
      "This error is likely caused internally from compiler.",
    );
    Logger.getInstance().printError(logSignalData);
  }
}
