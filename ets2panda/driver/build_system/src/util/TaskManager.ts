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

import { ChildProcess, fork } from 'child_process'
import * as os from 'os';

import { DEFAULT_WORKER_NUMS } from '../pre_define';
import { Logger, LogDataFactory, LogData } from '../logger';
import { Worker as Thread } from 'worker_threads';
import { WorkerMessageType, JobInfo } from '../types';
import { ErrorCode } from './error'
import { Graph, GraphNode } from './graph';

export interface Task<PayloadT> {
    id: string;
    payload: PayloadT;
    timeoutTimer?: NodeJS.Timeout;
    success?: boolean;
}

export interface WorkerInfo {
    worker: DriverWorker;
    id: number;
    currentTaskId?: string;
}

type OnWorkerExitCallback<PayloadT> = (
    workerInfo: WorkerInfo,
    task: Task<PayloadT>,
    code: number | null,
    signal: NodeJS.Signals | null
) => LogData;

interface WorkerMessage {
    type: WorkerMessageType,
    data: {
        taskId: string;
        error?: LogData;
    },
}

interface DriverWorker {
    on(msg: string, listener: (...args: any) => void): DriverWorker;
    send(msgType: string, data?: any): boolean;
    stop(...args: any): number;
    getId(): number;
    getWorkerPath(): string;
    spawnNewInstance(): DriverWorker;
}

export class DriverThread implements DriverWorker {
    private thread: Thread;
    private path: string;
    private args: any;

    constructor(workerPath: string, ...args: any) {
        this.path = workerPath;
        this.args = args;
        this.thread = new Thread(workerPath, ...args);
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
        (async (): Promise<void> => {
            await this.thread.terminate().then((value: number) => {
                res = value;
            })
        })();
        return res;
    }
    getId(): number {
        return this.thread.threadId;
    }
    getWorkerPath(): string {
        return this.path
    }
    spawnNewInstance(): DriverThread {
        return new DriverThread(this.path, ...this.args)
    }
}

export class DriverProcess implements DriverWorker {
    private process: ChildProcess;
    private path: string;
    private args: any;

    constructor(workerPath: string, ...args: any) {
        this.path = workerPath
        this.args = args;
        this.process = fork(workerPath, ...args);
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
    spawnNewInstance(): DriverProcess {
        return new DriverProcess(this.path, ...this.args)
    }
}

interface WorkerFactory {
    spawnWorker(): DriverWorker;
}

export class DriverProcessFactory implements WorkerFactory {
    private path: string;
    private args: any[];

    constructor(path: string, ...args: any) {
        this.path = path;
        this.args = args;
    }

    spawnWorker(): DriverProcess {
        return new DriverProcess(this.path, ...this.args)
    }
}

export class TaskManager<PayloadT extends JobInfo> {
    private workers: WorkerInfo[] = [];
    private idleWorkers: WorkerInfo[] = [];
    private taskQueue: Task<PayloadT>[] = [];
    private completedTasks: Task<PayloadT>[] = [];
    private runningTasks = new Map<string, Task<PayloadT>>();
    private maxWorkers = DEFAULT_WORKER_NUMS;
    private onWorkerExit: OnWorkerExitCallback<PayloadT>;
    private taskTimeoutMs: number;
    private logger: Logger;
    private isDeclgen: boolean;
    public buildGraph: Graph<PayloadT> = new Graph<PayloadT>();
    private completionResolve?: (success: boolean) => void;

    constructor(onWorkerExit: OnWorkerExitCallback<PayloadT>, declgen: boolean = false,
        maxWorkers?: number, taskTimeoutMs: number = 180000) {

        this.logger = Logger.getInstance();
        this.isDeclgen = declgen
        this.onWorkerExit = onWorkerExit;
        this.taskTimeoutMs = taskTimeoutMs;
        if (maxWorkers !== undefined) {
            this.maxWorkers = Math.min(maxWorkers, Math.max(os.cpus().length - 1, 1));
        }
        this.logger.printInfo(`Available workers: ${this.maxWorkers}`)
    }

    private tryDispatch(): void {
        while (this.taskQueue.length > 0 && this.idleWorkers.length > 0) {
            const task: Task<PayloadT> = this.taskQueue.shift()!;
            const workerInfo: WorkerInfo = this.idleWorkers.shift()!;
            this.assignTaskToWorker(task, workerInfo);
        }

        if (this.checkIfComplete()) {
            this.signalCompletion();
        }
    }

    private checkIfComplete(): boolean {
        const noRunningTasks = this.runningTasks.size === 0;
        const noQueuedTasks = this.taskQueue.length === 0;
        const allWorkersIdle = this.idleWorkers.length === this.maxWorkers;

        return noRunningTasks && noQueuedTasks && allWorkersIdle;
    }

    private signalCompletion(): void {
        const success = this.completedTasks.every(t => t.success === true);
        this.completionResolve?.(success);
    }

    private assignTaskToWorker(task: Task<PayloadT>, workerInfo: WorkerInfo): void {
        this.runningTasks.set(task.id, task);
        workerInfo.currentTaskId = task.id;

        task.timeoutTimer = setTimeout(() => {
            this.logger.printWarn(`Worker with id ${workerInfo.id} exceeded timeout. Stopping it...`)
            this.logger.printWarn(`Dropping task ${task.id}`)
            const logData = LogDataFactory.newInstance(
                this.isDeclgen ?
                    ErrorCode.BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER :
                    ErrorCode.BUILDSYSTEM_COMPILE_FAILED_IN_WORKER,
                `Task ${task.id} is not completed. Dropping it. Processed file is ${task.payload.fileInfo.input}`,
                `Worker ${workerInfo.id} exceeded timeout of ${this.taskTimeoutMs} ms`,
            )
            this.logger.printError(logData)
            this.handleTaskTimeout(workerInfo);
        }, this.taskTimeoutMs);

        this.logger.printDebug(`Dispatch task with id ${task.id} to worker ${workerInfo.id}`)
        workerInfo.worker.send(
            WorkerMessageType.ASSIGN_TASK,
            {
                taskId: task.id,
                payload: task.payload
            }
        );
    }

    private handleTaskTimeout(workerInfo: WorkerInfo): void {
        this.reconfigureWorker(workerInfo);
        this.tryDispatch();
    }

    private handleWorkerMessage(workerInfo: WorkerInfo, message: WorkerMessage): void {
        this.logger.printDebug(`WorkerMessage: ${JSON.stringify(message, null, 1)}`)
        switch (message.type) {
            case WorkerMessageType.ERROR_OCCURED:
                this.logErrorMessage(message);
                this.onTaskFailed(message.data.taskId, workerInfo);
                break;
            case WorkerMessageType.DECL_GENERATED:
                this.onDeclGenerated(message.data.taskId, workerInfo);
                break;
            case WorkerMessageType.ABC_COMPILED:
                this.onFileCompiled(message.data.taskId, workerInfo);
                break;
            default:
                break;
        }
    }

    private onTaskFailed(taskId: string, workerInfo: WorkerInfo): void {
        this.settleTask(taskId, true);
        workerInfo.currentTaskId = undefined;
        this.idleWorkers.push(workerInfo);
        this.tryDispatch();
    }

    private onFileCompiled(taskId: string, workerInfo: WorkerInfo): void {
        this.settleTask(taskId, false);
        workerInfo.currentTaskId = undefined;
        this.idleWorkers.push(workerInfo);
        this.tryDispatch();
    }

    private onDeclGenerated(taskId: string, workerInfo: WorkerInfo): void {
        // (1) Declgen-only mode: worker is now free for next task
        // (2) Compile mode: we can only release the worker until ABC compilation is done
        // in this case, declgen is only a signal to queue the next compilation task

        this.settleTask(taskId, false);
        if (this.isDeclgen) {
            // Declgen-only mode here
            workerInfo.currentTaskId = undefined;
            this.idleWorkers.push(workerInfo);
        }

        this.tryDispatch();
    }

    private handleWorkerError(error: Error): void {
        this.logger.printDebug('handleWorkerError')

        const logData = LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_COMPILE_FAILED_IN_WORKER,
            error.message
        )
        this.logger.printError(logData)
    }

    private handleWorkerExit(workerInfo: WorkerInfo, code: number | null, signal: NodeJS.Signals | null): void {
        this.logger.printDebug(`handleWorkerExit: code=${code}, signal=${signal}`);

        const taskId: string | undefined = workerInfo.currentTaskId;
        if (taskId) {
            const task = this.runningTasks.get(taskId);
            if (task) {
                this.logger.printError(this.onWorkerExit(workerInfo, task, code, signal));
                this.reconfigureWorker(workerInfo);
                this.tryDispatch();
            }
        }
    }

    public startWorkers(workerFactory: WorkerFactory): void {
        for (let i = 0; i < this.maxWorkers; i++) {
            const worker: DriverWorker = workerFactory.spawnWorker();

            this.logger.printDebug(`Spawned worker with id ${worker.getId()}`)

            const workerInfo: WorkerInfo = { worker, id: worker.getId(), currentTaskId: undefined };

            worker.on('message', (message: WorkerMessage) => {
                this.logger.printDebug(`Got ${message.type} message from worker ${workerInfo.id}`)
                this.handleWorkerMessage(workerInfo, message);
            });

            worker.on('exit', (code: number | null, signal: NodeJS.Signals | null) => {
                this.handleWorkerExit(workerInfo, code, signal);
            });

            worker.on('error', (error: Error) => {
                this.handleWorkerError(error);
                this.reconfigureWorker(workerInfo);
            });

            this.workers.push(workerInfo);
            this.idleWorkers.push(workerInfo);
        }
    }

    public initTaskQueue(): void {
        this.buildGraph.nodes.forEach((node: GraphNode<PayloadT>) => {
            if (node.predecessors.size === 0) {
                this.taskQueue.push({
                    id: node.id,
                    payload: node.data
                });
            }
        });
    }

    public async finish(): Promise<boolean> {
        const completionPromise = new Promise<boolean>((resolve) => {
            this.completionResolve = resolve;
        });

        this.tryDispatch();

        const success = await completionPromise;
        this.logger.printInfo('All tasks were completed');

        this.shutdownWorkers();
        this.logger.printInfo('All workers were shutdown')
        this.logger.printDebug('TaskManager.compile exit')

        return success;
    }

    private queueDependentTasks(taskId: string): void {
        const graphNode: GraphNode<PayloadT> = this.buildGraph.getNodeById(taskId);
        graphNode.descendants.forEach((descendant: string) => {
            const descendantNode = this.buildGraph.getNodeById(descendant);
            descendantNode.predecessors.delete(taskId);
            if (descendantNode.predecessors.size === 0) {
                this.taskQueue.push({
                    id: descendantNode.id,
                    payload: descendantNode.data
                });
                this.logger.printDebug(`[Declgen milestone] Added job ${descendant} to the queue`);
            } else {
                this.logger.printDebug(`[Declgen milestone] Job ${descendant} still has dependencies ${descendantNode.predecessors}`)
            }
        });
        this.logger.printDebug(`[Declgen milestone] Task [${taskId}] declgen completed, unlocked dependents`);
    }

    private settleTask(completedTaskId: string, failed: boolean = false): void {
        const task = this.runningTasks.get(completedTaskId);
        if (!task) {
            this.logger.printDebug(`Task [${completedTaskId}] has already been removed`)
            return;
        }
        if (task.timeoutTimer) {
            clearTimeout(task.timeoutTimer);
            task.timeoutTimer = undefined;
        }
        this.runningTasks.delete(completedTaskId);

        this.logger.printDebug(`Removed task [${completedTaskId}] from running tasks`)

        this.queueDependentTasks(completedTaskId);

        this.logger.printDebug(`Task [${completedTaskId}] is completed with status: ${!failed ? 'success' : 'failed'}`)
        task.success = !failed;
        this.completedTasks.push(task)
    }

    private reconfigureWorker(workerInfo: WorkerInfo): void {
        this.settleTask(workerInfo.currentTaskId!, true)
        workerInfo.currentTaskId = undefined;

        const worker = workerInfo.worker;
        worker.stop();

        const newWorker = worker.spawnNewInstance();
        workerInfo.worker = newWorker;
        workerInfo.id = newWorker.getId();

        this.logger.printDebug(`Spawned new worker with id ${newWorker.getId()}`);

        newWorker.on('message', (message: WorkerMessage) => {
            this.logger.printDebug(`Got ${message.type} message from worker ${workerInfo.id}`)
            this.handleWorkerMessage(workerInfo, message);
        });
        newWorker.on('exit', (code: number | null, signal: NodeJS.Signals | null) => {
            this.handleWorkerExit(workerInfo, code, signal);
        });
        newWorker.on('error', (error: Error) => {
            this.handleWorkerError(error);
            this.reconfigureWorker(workerInfo);
        });

        this.logger.printDebug(`Worker with id ${newWorker.getId()} is now idle`);
        this.idleWorkers.push(workerInfo);
    }

    private logErrorMessage(message: WorkerMessage): void {
        const err: LogData | undefined = message.data.error;
        if (!err) {
            return;
        }

        // Just to make LogData methods available
        const logData = new LogData(
            err.code,
            err.description,
            err.cause,
            err.position,
            err.solutions,
            err.moreInfo
        );

        this.logger.printError(logData);
    }

    public shutdownWorkers(): void {
        this.logger.printDebug('Shutdown workers...')
        this.workers.forEach((workerInfo) => {
            workerInfo.worker.stop();
        });
        this.workers = [];
        this.idleWorkers = [];
        this.runningTasks.clear();
        this.taskQueue = [];
    }
}
