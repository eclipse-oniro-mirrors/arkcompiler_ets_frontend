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

import * as Process from 'child_process';
import * as os from 'os';

import { DEFAULT_WORKER_NUMS } from '../pre_define';
import { Logger, LogData } from '../logger';
import { Worker as JSThreadWorker } from 'worker_threads';
import { CompileJobInfo } from '../types';
import { DriverError } from './error'

export interface Task<PayloadT> {
    id: string;
    payload: PayloadT;
    resolve: (result: true) => void;
    reject: (error: Object) => void;
    timeoutTimer?: NodeJS.Timeout;
}

export interface WorkerInfo {
    worker: DriverWorker;
    id: number;
    currentTaskId?: string;
    isIdle: boolean;
    isKilled: boolean;
}

type OnWorkerExitCallback<PayloadT> = (
    workerInfo: WorkerInfo,
    code: number | null,
    signal: NodeJS.Signals | null,
    runningTasks: Map<string, Task<PayloadT>>
) => void;

interface WorkerMessage {
    job: CompileJobInfo;
    success: boolean;
    shouldKill: boolean;
    error?: LogData;
}

interface DriverWorker {
    on(msg: string, listener: (...args: any) => void): DriverWorker;
    send(msgType: string, data?: any): boolean;
    stop(...args: any): number;
    getId(): number;
    getWorkerPath(): string;
    createNewInstance(workerPath: string, ...args: any): DriverWorker;
}

export class DriverThread implements DriverWorker {
    private thread: JSThreadWorker;
    private path: string;

    constructor(workerPath: string, ...args: any) {
        this.path = workerPath;
        this.thread = new JSThreadWorker(workerPath, ...args);
    }
    on(msg: string, listener: (...args: any) => void): DriverThread {
        this.thread.on(msg, listener);
        return this;
    }
    send(msgType: string, data?: any): boolean {
        this.thread.postMessage({ type: msgType, data: data });
        return true
    }
    stop(): number {
        let res = 0;
        (async () => {
            await this.thread.terminate().then((value: number) => { res = value; })
        })();
        return res;
    }
    getId(): number {
        return this.thread.threadId;
    }
    getWorkerPath(): string {
        return this.path
    }
    createNewInstance(workerPath: string, ...args: any): DriverThread {
        return new DriverThread(workerPath, ...args)
    }
}

export class DriverProcess implements DriverWorker {
    private process: Process.ChildProcess;
    private path: string;

    constructor(workerPath: string, ...args: any) {
        this.path = workerPath
        this.process = Process.fork(workerPath, ...args);
    }
    on(msg: string, listener: (...args: any) => void): DriverProcess {
        this.process.on(msg, listener);
        return this;
    }
    send(msgType: string, data?: any): boolean {
        return this.process.send({ type: msgType, data: data });
    }
    stop(): number {
        this.process.kill();
        return 0;
    }
    getId(): number {
        return this.process.pid!;
    }
    getWorkerPath(): string {
        return this.path
    }
    createNewInstance(workerPath: string, ...args: any): DriverProcess {
        return new DriverProcess(workerPath, ...args)
    }
}

class WorkerFactory {
    static spawnWorker<WorkerT extends DriverWorker>(type: { new(...args: any): WorkerT; }, ...args: any): WorkerT {
        return new type(...args);
    }
}

export class TaskManager<PayloadT> {
    private workers: WorkerInfo[] = [];
    private idleWorkers: WorkerInfo[] = [];
    private taskQueue: Task<PayloadT>[] = [];
    private runningTasks = new Map<string, Task<PayloadT>>();
    private maxWorkers = DEFAULT_WORKER_NUMS;
    private onWorkerExit: OnWorkerExitCallback<PayloadT>;
    private taskTimeoutMs: number;

    constructor(onWorkerExit: OnWorkerExitCallback<PayloadT>,
        maxWorkers?: number, taskTimeoutMs: number = 180000) {
        const cpuCount = Math.max(os.cpus().length - 1, 1);

        this.onWorkerExit = onWorkerExit;
        this.taskTimeoutMs = taskTimeoutMs;

        if (maxWorkers) {
            this.maxWorkers = Math.min(maxWorkers, cpuCount);
        }
    }

    private handleWorkerMessage(workerInfo: WorkerInfo, message: WorkerMessage): void {
        const { job, success } = message;
        if (!success) {
            this.logErrorMessage(message);
        }
        this.settleTask(job.id, success);
        workerInfo.currentTaskId = undefined;
        this.idleWorkers.push(workerInfo);
        this.dispatchNext();
    }

    private handleWorkerExit(workerInfo: WorkerInfo, code: number | null, signal: NodeJS.Signals | null): void {
        const taskId: string | undefined = workerInfo.currentTaskId;
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

    public startWorkers<WorkerT extends DriverWorker>(type: { new(...args: any): WorkerT; }, ...args: any): void {
        for (let i = 0; i < this.maxWorkers; i++) {
            const worker: WorkerT = WorkerFactory.spawnWorker<WorkerT>(type, ...args);

            const workerInfo: WorkerInfo = { worker, id: worker.getId(), isKilled: false, isIdle: true };

            worker.on('message', (message: WorkerMessage) => {
                this.handleWorkerMessage(workerInfo, message);
            });

            worker.on('exit', (code: number, signal) => {
                this.handleWorkerExit(workerInfo, code, signal);
            });

            worker.on('error', (error: DriverError) => {
                this.shutdownWorkers();
                Logger.getInstance().printErrorAndExit(error.logData);
            });

            this.workers.push(workerInfo);
            this.idleWorkers.push(workerInfo);
        }
    }


    private dispatchNext(): void {
        while (this.taskQueue.length > 0 && this.idleWorkers.length > 0) {
            const task: Task<PayloadT> = this.taskQueue.shift()!;
            const workerInfo: WorkerInfo = this.idleWorkers.shift()!;

            this.runningTasks.set(task.id, task);
            workerInfo.currentTaskId = task.id;

            task.timeoutTimer = setTimeout(() => {
                this.taskQueue.push(task);
                workerInfo.currentTaskId = undefined;
                workerInfo.worker.stop();
                this.reconfigureWorker(workerInfo);
                this.dispatchNext();
            }, this.taskTimeoutMs);

            workerInfo.worker.send('ASSIGN_TASK', { id: task.id, payload: task.payload });
        }
    }

    private settleTask(taskId: string, success: boolean, error?: string): void {
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

    private handleSignals(workerInfo: WorkerInfo, signal: NodeJS.Signals | null): void {
        if (!signal) {
            return;
        }
        switch (signal) {
            case 'SIGTERM':
                break;
            case 'SIGSEGV':
                this.reconfigureWorker(workerInfo);
                break;
            default:
                break;
        }
    }

    private reconfigureWorker(workerInfo: WorkerInfo): void {
        const worker = workerInfo.worker
        worker.createNewInstance(worker.getWorkerPath(), {
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
            this.shutdownWorkers();
            Logger.getInstance().printErrorAndExit(logData);
        } else {
            Logger.getInstance().printError(logData);
        }
    }

    private getWorkerExitReason(code: number | null, signal: NodeJS.Signals | null):
        string | undefined {
        if (signal && signal !== 'SIGKILL') {
            return `Worker killed by signal ${signal}`;
        }
        return code !== 0 ? `Worker exited with code ${code}` : undefined;
    }

    public submitJob(id: string, payload: PayloadT): Promise<true> {
        return new Promise<true>((resolve, reject) => {
            const task: Task<PayloadT> = {
                id,
                payload,
                resolve,
                reject,
            };
            this.taskQueue.push(task);
            this.dispatchNext();
        });
    }

    public async shutdownWorkers(): Promise<void> {
        await Promise.all(this.workers.map((workerInfo) =>
            new Promise<void>((res) => {
                workerInfo.isKilled = true;
                workerInfo.worker.stop();
                res();
            })
        ));
        this.workers = [];
        this.idleWorkers = [];
        this.runningTasks.clear();
        this.taskQueue = [];
    }
}
