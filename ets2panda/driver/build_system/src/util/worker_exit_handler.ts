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

import { ErrorCode, DriverError } from '../util/error';
import { getEs2pandaPath } from '../init/process_build_config';
import { LogData, LogDataFactory, Logger } from '../logger';
import { ProcessCompileTask } from '../types';
import { Task, WorkerInfo } from './TaskManager';

export function handleCompileProcessWorkerExit(
    workerInfo: WorkerInfo,
    code: number | null,
    signal: NodeJS.Signals | null,
    runningTasks: Map<string, Task<ProcessCompileTask>>
): void {
    if (!code || code === 0) {
        return
    }
    const taskId: string | undefined = workerInfo.currentTaskId;
    const payload: ProcessCompileTask | undefined = runningTasks.get(taskId!)?.payload;
    if (!payload) {
        return;
    }
    const es2pandPath = getEs2pandaPath(payload.buildConfig);
    const cmd = [
        es2pandPath,
        '--arktsconfig', payload.job.compileFileInfo.arktsConfigFile,
        '--output', payload.job.compileFileInfo.outputFilePath,
        payload.job.compileFileInfo.inputFilePath
    ];

    throw new DriverError(
        LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_COMPILE_FAILED_IN_WORKER,
            `Compile file ${payload.job.compileFileInfo.inputFilePath} crashed (exit code ${code})`,
            '',
            '',
            [`Please try to run command locally : ${cmd.join(' ')}`]
        )
    );
}

export function handleDeclgenWorkerExit(
    workerInfo: WorkerInfo,
    code: number | null,
    signal: NodeJS.Signals | null,
): void {

    if (code !== 0) {
        throw new DriverError(
            LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER,
                `Declgen crashed (exit code ${code})`,
                'This error is likely caused internally from compiler.',
            )
        );
    }
    if (signal !== 'SIGTERM') {
        throw new DriverError(
            LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER,
                `Declgen crashed (exit signal ${signal})`,
                "This error is likely caused internally from compiler.",
            )
        );
    }
}
