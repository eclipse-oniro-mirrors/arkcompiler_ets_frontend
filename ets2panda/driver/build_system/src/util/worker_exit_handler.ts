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

import { ErrorCode } from '../util/error';
import { getEs2pandaPath } from '../init/process_build_config';
import { LogDataFactory, LogData } from '../logger';
import { ProcessCompileTask, ProcessDeclgenV1Task, JobInfo } from '../types';
import { Task, WorkerInfo } from './TaskManager';

function getErrorMessage<PayloadT extends JobInfo>(
    workerInfo: WorkerInfo,
    task: Task<PayloadT>,
    code: number | null,
    signal: NodeJS.Signals | null
): string {
    if (signal) {
        switch (signal) {
            case 'SIGSEGV':
                return `Worker [ID:${workerInfo.id}] caught SIGSEGV signal`;
            case 'SIGKILL':
                return `Worker [ID:${workerInfo.id}] was killed by signal ${signal}`;
            default:
                return `Signal ${signal} was sent to the worker [${workerInfo.id}]`;
        }
    }

    if (code && code !== 0) {
        return `Failed to compile ${task.payload.fileList[0]}. Exit code ${code}`;
    }

    return 'Worker exited unexpectedly';
}

export function handleCompileProcessWorkerExit(
    workerInfo: WorkerInfo,
    task: Task<ProcessCompileTask>,
    code: number | null,
    signal: NodeJS.Signals | null
): LogData {
    const es2pandPath = getEs2pandaPath(task.payload.buildConfig);
    const cmd = [
        es2pandPath,
        '--arktsconfig', task.payload.fileInfo.arktsConfig,
        '--output', task.payload.fileInfo.output,
        task.payload.fileInfo.input
    ];

    return LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_FAILED_IN_WORKER,
        getErrorMessage(workerInfo, task, code, signal),
        'This error is likely caused internally from compiler.',
        task.payload.fileList[0],
        [`Run locally: ${cmd.join(' ')}`]
    )
}

export function handleDeclgenWorkerExit(
    workerInfo: WorkerInfo,
    task: Task<ProcessDeclgenV1Task>,
    code: number | null,
    signal: NodeJS.Signals | null
): LogData {
    return LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER,
        getErrorMessage(workerInfo, task, code, signal),
        'This error is likely caused internally from compiler.',
    )
}
