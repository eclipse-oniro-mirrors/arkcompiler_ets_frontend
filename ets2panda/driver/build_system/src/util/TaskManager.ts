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

import { fork, ChildProcess } from 'child_process';
import * as os from 'os';

import { DEFAULT_WOKER_NUMS } from '../pre_define';
import { createTaskId } from './utils';
import { LogData, Logger } from '../logger';

export interface Task<T> {
    id: string;
    payload: T;
    resolve: (result: true) => void;
    reject: (error: Object) => void;
    timeoutTimer?: NodeJS.Timeout;
}

export interface WorkerInfo {
    worker: ChildProcess;
    id: number;
    currentTaskId?: string;
    isKilled: boolean;
}

type OnWorkerExitCallback<T> = (
    workerInfo: WorkerInfo,
    code: number | null,
    signal: NodeJS.Signals | null,
    runningTasks: Map<string, Task<T>>
) => void;

interface WorkerMessage {
    id: string;
    success: boolean;
    shouldKill: boolean;
    error?: LogData;
}

export class TaskManager<T> {
    private workers: WorkerInfo[] = [];
    private idleWorkers: WorkerInfo[] = [];
    private taskQueue: Task<T>[] = [];
    private runningTasks = new Map<string, Task<T>>();
    private maxWorkers = DEFAULT_WOKER_NUMS;
    private workerPath: string;
    private onWorkerExit: OnWorkerExitCallback<T>;
    private taskTimeoutMs: number;

    constructor(workerPath: string, onWorkerExit: OnWorkerExitCallback<T>,
        maxWorkers?: number, taskTimeoutMs: number = 180000) {
        const cpuCount = Math.max(os.cpus().length - 1, 1);

        this.workerPath = workerPath;
        this.onWorkerExit = onWorkerExit;
        this.taskTimeoutMs = taskTimeoutMs;

        if (maxWorkers !== undefined) {
            this.maxWorkers = Math.min(maxWorkers, cpuCount);
        } else {
            this.maxWorkers = DEFAULT_WOKER_NUMS;
        }
    }

    public startWorkers(): void {
        for (let i = 0; i < this.maxWorkers; i++) {
            const worker = fork(this.workerPath, [], {
                stdio: ['inherit', 'inherit', 'inherit', 'ipc']
            });

            const workerInfo: WorkerInfo = { worker, id: i, isKilled: false };

            worker.on('message', (message: WorkerMessage) => {
                this.handleWorkerMessage(workerInfo, message);
            });

            worker.on('exit', (code, signal) => {
                this.handleWorkerExit(workerInfo, code, signal);
            });

            this.workers.push(workerInfo);
            this.idleWorkers.push(workerInfo);
        }

        this.dispatchNext();
    }


    private settleTask(taskId: string, success: boolean, error?: string) {
        const task = this.runningTasks.get(taskId);
        if (!task) {
            return;
        }
        if (task.timeoutTimer) {
            clearTimeout(task.timeoutTimer);
            task.timeoutTimer = undefined;
        }
        if (success) {
            task.resolve(true);
        }
        else {
            task.reject(error ?? new Error(error));
        }
        this.runningTasks.delete(taskId);
    }

    private handleSignals(workerInfo: WorkerInfo, signal: NodeJS.Signals | null) {
        if (!signal) {
            return;
        }
        switch (signal) {
            case "SIGTERM":
                break;
            case "SIGSEGV":
                this.reconfigureWorker(workerInfo);
                break;
            default:
                break;
        }
    }

    private reconfigureWorker(workerInfo: WorkerInfo) {
        const worker = fork(this.workerPath, [], {
            stdio: ['inherit', 'inherit', 'inherit', 'ipc']
        });
        workerInfo.currentTaskId = undefined;
        workerInfo.worker = worker;
        worker.on('message', (message: WorkerMessage) => {
            this.handleWorkerMessage(workerInfo, message);
        });
        worker.on('exit', (code, signal) => {
            this.handleWorkerExit(workerInfo, code, signal);
        });
        this.idleWorkers.push(workerInfo);
    }

    private handleWorkerExit(workerInfo: WorkerInfo, code: number | null, signal: NodeJS.Signals | null) {
        const taskId = workerInfo.currentTaskId;
        if (taskId) {
            const success = code === 0 && !signal;
            const reason = this.getWorkerExitReason(code, signal);
            this.settleTask(taskId, success, reason);
        }

        this.handleSignals(workerInfo, signal);

        if (this.onWorkerExit) {
            this.onWorkerExit(workerInfo, code, signal, this.runningTasks);
        }
    }

    private logErrorMessage(message: WorkerMessage): void {
        const err = message.error;
        if (!err) {
            return;
        }
        const logData = new LogData(
            err.code,
            err.description,
            err.cause,
            err.position,
            err.solutions,
            err.moreInfo
        );
        if (message.shouldKill) {
            this.shutdown();
            Logger.getInstance().printErrorAndExit(logData);
        } else {
            Logger.getInstance().printError(logData);
        }
    }

    private handleWorkerMessage(workerInfo: WorkerInfo, message: WorkerMessage) {
        const { id, success } = message;
        if (!success) {
            this.logErrorMessage(message);
        }
        this.settleTask(id, success);
        workerInfo.currentTaskId = undefined;
        this.idleWorkers.push(workerInfo);
        this.dispatchNext();
    }

    private getWorkerExitReason(code: number | null, signal: NodeJS.Signals | null):
        string | undefined {
        if (signal && signal !== 'SIGKILL') {
            return `Worker killed by signal ${signal}`;
        }
        return code !== 0 ? `Worker exited with code ${code}` : undefined;
    }

    private dispatchNext(): void {
        while (this.taskQueue.length > 0 && this.idleWorkers.length > 0) {
            const task = this.taskQueue.shift()!;
            const workerInfo = this.idleWorkers.shift()!;

            this.runningTasks.set(task.id, task);
            workerInfo.currentTaskId = task.id;

            task.timeoutTimer = setTimeout(() => {
                this.taskQueue.push(task);
                workerInfo.currentTaskId = undefined;
                workerInfo.worker.kill();
                this.reconfigureWorker(workerInfo);
                this.dispatchNext();
            }, this.taskTimeoutMs);

            workerInfo.worker.send({ id: task.id, payload: task.payload });
        }
    }

    public submitTask(payload: T): Promise<true> {
        return new Promise<true>((resolve, reject) => {
            const task: Task<T> = {
                id: createTaskId(),
                payload,
                resolve,
                reject,
            };
            this.taskQueue.push(task);
            this.dispatchNext();
        });
    }

    public async shutdown(): Promise<void> {
        await Promise.all(this.workers.map((workerInfo) =>
            new Promise<void>((res) => {
                workerInfo.isKilled = true;
                workerInfo.worker.kill();
                res();
            })
        ));
        this.workers = [];
        this.idleWorkers = [];
        this.runningTasks.clear();
        this.taskQueue = [];
    }
}
