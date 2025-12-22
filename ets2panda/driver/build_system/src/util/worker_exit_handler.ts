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

function getDeclgenErrorMessage<PayloadT extends JobInfo>(
    workerInfo: WorkerInfo,
    task: Task<PayloadT>,
    code: number | null,
    signal: NodeJS.Signals | null
): string {
    if (signal) {
        switch (signal) {
            case 'SIGSEGV':
                return `Declgen Worker caught SIGSEGV signal. Processed file is ${task.payload.fileInfo.input}.`;
            case 'SIGKILL':
                return `Declgen Worker was killed by signal ${signal}`;
            default:
                return `Signal ${signal} was sent to the declgen worker. Processed file is ${task.payload.fileInfo.input}.`;
        }
    }

    if (code && code !== 0) {
        return `Declgen worker crashed when process file ${task.payload.fileInfo.input}. Exit code ${code}(0x${code.toString(16)})`;
    }

    return `Worker [ID:${workerInfo.id}] exited unexpectedly`;
}

function getCompileErrorMessage<PayloadT extends JobInfo>(
    workerInfo: WorkerInfo,
    task: Task<PayloadT>,
    code: number | null,
    signal: NodeJS.Signals | null
): string {
    if (signal) {
        switch (signal) {
            case 'SIGSEGV':
                return `Failed to compile file ${task.payload.fileInfo.input}. Compiler worker caught SIGSEGV signal.`;
            case 'SIGKILL':
                return `Failed to compile file ${task.payload.fileInfo.input}. Compiler worker was killed by signal ${signal}`;
            default:
                return `Signal ${signal} was sent to the compiler worker. Processed file is ${task.payload.fileInfo.input}.`;
        }
    }

    if (code && code !== 0) {
        return `Failed to compile file ${task.payload.fileInfo.input}. Exit code ${code}(0x${code.toString(16)})`;
    }

    return `Worker [ID:${workerInfo.id}] exited unexpectedly`;
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
        getCompileErrorMessage(workerInfo, task, code, signal),
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
        getDeclgenErrorMessage(workerInfo, task, code, signal),
        'This error is likely caused internally from compiler.',
    )
}
