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

import { fork } from 'node:child_process';
import type { ChildProcess, ForkOptions } from 'node:child_process';
import * as os from 'node:os';

import { LogData } from '@interop-toolkits/common';
import type { LogDataInit } from '@interop-toolkits/common';

import { logger } from '../../logger';
import { DagScheduler } from './dagScheduler';
import { ErrorCode } from '../../errors';
import { Graph } from './graph';

type ForkArgs = readonly [args?: readonly string[], options?: ForkOptions];

const DEFAULT_WORKER_NUMS = 3;
const DEFAULT_TASK_TIMEOUT_MS = 180000;
// process root cluster firstly or not
const ENABLE_DISPATCH_ROOT_CLUSTER_FIRST = true;

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
  taskResult?: boolean;
}

type OnWorkerExitCallback<PayloadT> = (
  workerInfo: WorkerInfo,
  task: Task<PayloadT>,
  code: number | null,
  signal: NodeJS.Signals | null,
) => LogData;

export enum WorkerMessageType {
  DECL_GENERATED = 'DECL_GENERATED',
  ERROR_OCCURED = 'ERROR_OCCURED',
  ASSIGN_TASK = 'ASSIGN_TASK',
  TASK_FINISHED = 'TASK_FINISHED',
  LOG = 'LOG',
}

export enum LogLevel {
  INFO = 'INFO',
  WARN = 'WARN',
  DEBUG = 'DEBUG',
  ERROR = 'ERROR',
  ERROR_AND_EXIT = 'ERROR_AND_EXIT',
}

interface WorkerDeclGeneratedMessage {
  type: WorkerMessageType.DECL_GENERATED;
  data: { taskId: string };
}

interface WorkerErrorMessage {
  type: WorkerMessageType.ERROR_OCCURED;
  data: { taskId: string; error: LogDataInit | LogDataInit[] };
}

interface WorkerTaskFinishedMessage {
  type: WorkerMessageType.TASK_FINISHED;
  data?: { taskId?: string };
}

interface WorkerTextLogMessage {
  type: WorkerMessageType.LOG;
  data: { level: LogLevel.INFO | LogLevel.WARN | LogLevel.DEBUG; message: string };
}

interface WorkerErrorLogMessage {
  type: WorkerMessageType.LOG;
  data: { level: LogLevel.ERROR | LogLevel.ERROR_AND_EXIT; error: LogDataInit };
}

type WorkerLogMessage = WorkerTextLogMessage | WorkerErrorLogMessage;

type WorkerMessage = WorkerDeclGeneratedMessage | WorkerErrorMessage | WorkerTaskFinishedMessage | WorkerLogMessage;

export interface DriverWorker {
  on(msg: 'message', listener: (message: unknown) => void): DriverWorker;
  on(msg: 'exit', listener: (code: number | null, signal: NodeJS.Signals | null) => void): DriverWorker;
  on(msg: 'error', listener: (error: Error) => void): DriverWorker;
  send(msgType: string, data?: unknown, callback?: (error: Error | null) => void): void;
  stop(): number;
  getId(): number;
  getWorkerPath(): string;
  spawnNewInstance(): DriverWorker;
}

export class DriverProcess implements DriverWorker {
  private readonly process: ChildProcess;
  private readonly path: string;
  private readonly args: ForkArgs;

  public constructor(workerPath: string, ...args: ForkArgs) {
    this.path = workerPath;
    this.args = args;
    this.process = fork(workerPath, ...args);
  }

  public on(msg: 'message', listener: (message: unknown) => void): DriverProcess;
  public on(msg: 'exit', listener: (code: number | null, signal: NodeJS.Signals | null) => void): DriverProcess;
  public on(msg: 'error', listener: (error: Error) => void): DriverProcess;
  public on(msg: string, listener: (...args: never[]) => void): DriverProcess {
    this.process.on(msg, (...args) => listener(...(args as never[])));
    return this;
  }

  public send(msgType: string, data?: unknown, callback?: (error: Error | null) => void): void {
    this.process.send({ type: msgType, data }, (error) => {
      callback?.(error);
    });
  }

  public stop(): number {
    this.process.kill();
    return 0;
  }

  public getId(): number {
    return this.process.pid!;
  }

  public getWorkerPath(): string {
    return this.path;
  }

  public spawnNewInstance(): DriverProcess {
    return new DriverProcess(this.path, ...this.args);
  }
}

export interface WorkerFactory {
  spawnWorker(): DriverWorker;
}

enum ManagerState {
  CREATED,
  STARTED,
  RUNNING,
  STOPPING,
  FINISHED,
}

export class DriverProcessFactory implements WorkerFactory {
  private readonly path: string;
  private readonly args: ForkArgs;

  public constructor(path: string, ...args: ForkArgs) {
    this.path = path;
    this.args = args;
  }

  public spawnWorker(): DriverProcess {
    return new DriverProcess(this.path, ...this.args);
  }
}

export class TaskManager<PayloadT> {
  private workers: WorkerInfo[] = [];
  private idleWorkers: WorkerInfo[] = [];
  private readonly runningTasks = new Map<string, Task<PayloadT>>();
  private maxWorkers = DEFAULT_WORKER_NUMS;
  private readonly onWorkerExit: OnWorkerExitCallback<PayloadT>;
  private readonly taskTimeoutMs: number;
  public buildGraph: Graph<PayloadT> = new Graph<PayloadT>();
  private scheduler?: DagScheduler<PayloadT>;
  private completionResolve?: (success: boolean) => void;
  private completionSignaled = false;
  private state = ManagerState.CREATED;

  public constructor(
    onWorkerExit: OnWorkerExitCallback<PayloadT>,
    maxWorkers?: number,
    taskTimeoutMs: number = DEFAULT_TASK_TIMEOUT_MS,
  ) {
    this.onWorkerExit = onWorkerExit;
    if (!Number.isFinite(taskTimeoutMs) || taskTimeoutMs <= 0) {
      throw new RangeError(`taskTimeoutMs must be positive, got ${taskTimeoutMs}`);
    }
    this.taskTimeoutMs = taskTimeoutMs;
    if (maxWorkers !== undefined) {
      if (!Number.isInteger(maxWorkers) || maxWorkers < 1) {
        throw new RangeError(`maxWorkers must be a positive integer, got ${maxWorkers}`);
      }
      this.maxWorkers = Math.min(maxWorkers, Math.max(os.cpus().length - 1, 1));
    }
    logger.printInfo(`Available workers: ${this.maxWorkers}`);
  }

  public startWorkers(workerFactory: WorkerFactory): void {
    if (this.state !== ManagerState.CREATED) {
      throw new Error('TaskManager workers can only be started once');
    }
    try {
      for (let i = 0; i < this.maxWorkers; i++) {
        const worker: DriverWorker = workerFactory.spawnWorker();
        logger.printDebug(`Spawned worker with id ${worker.getId()}`);

        const workerInfo: WorkerInfo = { worker, id: worker.getId(), currentTaskId: undefined };
        this.attachWorker(workerInfo, worker);
        this.workers.push(workerInfo);
        this.idleWorkers.push(workerInfo);
      }
    } catch (error) {
      this.shutdownWorkers();
      throw error;
    }
    this.state = ManagerState.STARTED;
  }

  public initTaskQueue(): void {
    if (this.state !== ManagerState.STARTED || this.scheduler) {
      throw new Error('TaskManager task queue can only be initialized once after workers are started');
    }
    this.scheduler = new DagScheduler(this.buildGraph, ENABLE_DISPATCH_ROOT_CLUSTER_FIRST);
  }

  public async finish(): Promise<boolean> {
    if (this.state !== ManagerState.STARTED || !this.scheduler) {
      throw new Error('TaskManager must start workers and initialize its task queue before finish()');
    }
    this.state = ManagerState.RUNNING;
    const completionPromise = new Promise<boolean>((resolve) => {
      this.completionResolve = resolve;
    });

    this.tryDispatch();

    const success = await completionPromise;
    logger.printInfo('All tasks were completed');

    this.shutdownWorkers();
    logger.printInfo('All workers were shutdown');
    logger.printDebug('TaskManager.finish exit');

    return success;
  }

  public shutdownWorkers(): void {
    if (this.state === ManagerState.FINISHED) {
      return;
    }
    this.state = ManagerState.STOPPING;
    logger.printDebug('Shutdown workers...');
    for (const task of this.runningTasks.values()) {
      if (task.timeoutTimer) {
        clearTimeout(task.timeoutTimer);
        task.timeoutTimer = undefined;
      }
    }
    this.workers.forEach((workerInfo) => {
      workerInfo.worker.stop();
    });
    this.workers = [];
    this.idleWorkers = [];
    this.runningTasks.clear();
    if (!this.completionSignaled) {
      this.completionSignaled = true;
      this.completionResolve?.(false);
    }
    this.state = ManagerState.FINISHED;
  }

  private tryDispatch(): void {
    if (this.state !== ManagerState.RUNNING) {
      return;
    }
    while (this.idleWorkers.length > 0) {
      const node = this.scheduler!.takeNext();
      if (!node) {
        break;
      }
      const workerInfo = this.idleWorkers.shift()!;
      this.assignTaskToWorker({ id: node.id, payload: node.data }, workerInfo);
    }

    if (this.checkIfComplete()) {
      this.signalCompletion();
    }
  }

  private checkIfComplete(): boolean {
    return (
      this.scheduler!.isComplete() && this.runningTasks.size === 0 && this.idleWorkers.length === this.workers.length
    );
  }

  private signalCompletion(): void {
    if (this.completionSignaled) {
      return;
    }
    this.completionSignaled = true;
    this.completionResolve?.(this.scheduler!.isSuccessful());
  }

  private assignTaskToWorker(task: Task<PayloadT>, workerInfo: WorkerInfo): void {
    this.runningTasks.set(task.id, task);
    workerInfo.currentTaskId = task.id;
    workerInfo.taskResult = undefined;

    task.timeoutTimer = setTimeout(() => {
      logger.printWarn(`Worker with id ${workerInfo.id} exceeded timeout. Stopping it...`);
      logger.printWarn(`Dropping task ${task.id}`);
      const logData = new LogData({
        code: ErrorCode.STATIC_WORKER_TASK_TIMEOUT,
        description: `Task ${task.id} is not completed. Dropping it.`,
        cause: `Worker ${workerInfo.id} exceeded timeout of ${this.taskTimeoutMs} ms`,
      });
      logger.printError(logData);
      this.replaceWorker(workerInfo);
      this.tryDispatch();
    }, this.taskTimeoutMs);

    logger.printDebug(`Dispatch task with id ${task.id} to worker ${workerInfo.id}`);
    try {
      const sourceWorker = workerInfo.worker;
      sourceWorker.send(
        WorkerMessageType.ASSIGN_TASK,
        {
          taskId: task.id,
          payload: task.payload,
        },
        (error) => {
          if (!error || !this.isCurrentWorker(workerInfo, sourceWorker) || workerInfo.currentTaskId !== task.id) {
            return;
          }
          this.handleWorkerError(error);
          this.replaceWorker(workerInfo);
          this.tryDispatch();
        },
      );
    } catch (error) {
      this.handleWorkerError(error instanceof Error ? error : new Error(String(error)));
      this.replaceWorker(workerInfo);
      this.tryDispatch();
    }
  }

  private handleWorkerMessage(workerInfo: WorkerInfo, message: unknown): void {
    if (!isWorkerMessage(message)) {
      this.handleWorkerError(new Error(`Worker ${workerInfo.id} sent an invalid message`));
      this.replaceWorker(workerInfo);
      this.tryDispatch();
      return;
    }
    logger.printDebug(`WorkerMessage: ${JSON.stringify(message, null, 1)}`);
    switch (message.type) {
      case WorkerMessageType.LOG:
        this.handleWorkerLog(message);
        break;
      case WorkerMessageType.ERROR_OCCURED:
        this.logErrorMessages(message.data.error);
        this.recordTaskResult(workerInfo, message.data.taskId, false);
        break;
      case WorkerMessageType.DECL_GENERATED:
        this.recordTaskResult(workerInfo, message.data.taskId, true);
        break;
      case WorkerMessageType.TASK_FINISHED:
        this.onTaskFinished(workerInfo, message.data?.taskId);
        break;
      default:
        break;
    }
  }

  private handleWorkerLog(message: WorkerLogMessage): void {
    switch (message.data.level) {
      case LogLevel.INFO:
        logger.printInfo(message.data.message);
        break;
      case LogLevel.WARN:
        logger.printWarn(message.data.message);
        break;
      case LogLevel.DEBUG:
        logger.printDebug(message.data.message);
        break;
      case LogLevel.ERROR:
        this.logErrorMessage(message.data.error);
        break;
      case LogLevel.ERROR_AND_EXIT:
        this.logErrorMessage(message.data.error, true);
        break;
      default:
        break;
    }
  }

  private onTaskFinished(workerInfo: WorkerInfo, messageTaskId?: string): void {
    const taskId = workerInfo.currentTaskId;
    if (!taskId || (messageTaskId !== undefined && messageTaskId !== taskId)) {
      this.handleWorkerError(new Error(`Worker ${workerInfo.id} finished an unexpected task`));
      this.replaceWorker(workerInfo);
      this.tryDispatch();
      return;
    }
    this.settleTask(workerInfo, workerInfo.taskResult === true);
    this.releaseWorker(workerInfo);
    this.tryDispatch();
  }

  private recordTaskResult(workerInfo: WorkerInfo, taskId: string, success: boolean): void {
    if (workerInfo.currentTaskId !== taskId || !this.runningTasks.has(taskId)) {
      this.handleWorkerError(new Error(`Worker ${workerInfo.id} reported an unexpected task ${taskId}`));
      this.replaceWorker(workerInfo);
      this.tryDispatch();
      return;
    }
    if (workerInfo.taskResult !== undefined && workerInfo.taskResult !== success) {
      this.handleWorkerError(new Error(`Worker ${workerInfo.id} reported conflicting results for task ${taskId}`));
      this.replaceWorker(workerInfo);
      this.tryDispatch();
      return;
    }
    workerInfo.taskResult = success;
  }

  private handleWorkerError(error: Error): void {
    logger.printDebug('handleWorkerError');

    const logData = new LogData({
      code: ErrorCode.STATIC_WORKER_PROCESS_FAILED,
      description: error.message,
    });
    logger.printError(logData);
  }

  private handleWorkerExit(workerInfo: WorkerInfo, code: number | null, signal: NodeJS.Signals | null): void {
    logger.printDebug(`handleWorkerExit: code=${code}, signal=${signal}`);

    const taskId = workerInfo.currentTaskId;
    if (taskId) {
      const task = this.runningTasks.get(taskId);
      if (task) {
        try {
          logger.printError(this.onWorkerExit(workerInfo, task, code, signal));
        } catch (error) {
          this.handleWorkerError(error instanceof Error ? error : new Error(String(error)));
        }
      }
    }
    this.replaceWorker(workerInfo);
    this.tryDispatch();
  }

  private settleTask(workerInfo: WorkerInfo, success: boolean): void {
    const taskId = workerInfo.currentTaskId;
    if (!taskId) {
      return;
    }
    const task = this.runningTasks.get(taskId);
    if (!task) {
      logger.printDebug(`Task [${taskId}] has already been removed`);
      return;
    }
    if (task.timeoutTimer) {
      clearTimeout(task.timeoutTimer);
      task.timeoutTimer = undefined;
    }
    this.runningTasks.delete(taskId);
    task.success = success;
    this.scheduler!.complete(taskId, success);
    workerInfo.currentTaskId = undefined;
    workerInfo.taskResult = undefined;
    logger.printDebug(`Task [${taskId}] is completed with status: ${success ? 'success' : 'failed'}`);
  }

  private replaceWorker(workerInfo: WorkerInfo): void {
    if (this.state === ManagerState.STOPPING || this.state === ManagerState.FINISHED) {
      return;
    }
    this.removeIdleWorker(workerInfo);
    this.settleTask(workerInfo, false);

    const oldWorker = workerInfo.worker;
    let newWorker: DriverWorker;
    try {
      newWorker = oldWorker.spawnNewInstance();
    } catch (error) {
      this.handleWorkerError(error instanceof Error ? error : new Error(String(error)));
      this.shutdownWorkers();
      return;
    }

    workerInfo.worker = newWorker;
    workerInfo.id = newWorker.getId();
    this.attachWorker(workerInfo, newWorker);
    oldWorker.stop();
    this.releaseWorker(workerInfo);
    logger.printDebug(`Replaced worker with new id ${newWorker.getId()}`);
  }

  private attachWorker(workerInfo: WorkerInfo, sourceWorker: DriverWorker): void {
    sourceWorker.on('message', (message: unknown) => {
      if (!this.isCurrentWorker(workerInfo, sourceWorker)) {
        return;
      }
      this.handleWorkerMessage(workerInfo, message);
    });
    sourceWorker.on('exit', (code: number | null, signal: NodeJS.Signals | null) => {
      if (!this.isCurrentWorker(workerInfo, sourceWorker)) {
        return;
      }
      this.handleWorkerExit(workerInfo, code, signal);
    });
    sourceWorker.on('error', (error: Error) => {
      if (!this.isCurrentWorker(workerInfo, sourceWorker)) {
        return;
      }
      this.handleWorkerError(error);
      this.replaceWorker(workerInfo);
      this.tryDispatch();
    });
  }

  private isCurrentWorker(workerInfo: WorkerInfo, sourceWorker: DriverWorker): boolean {
    return (
      workerInfo.worker === sourceWorker && this.state !== ManagerState.STOPPING && this.state !== ManagerState.FINISHED
    );
  }

  private releaseWorker(workerInfo: WorkerInfo): void {
    if (!this.idleWorkers.includes(workerInfo)) {
      this.idleWorkers.push(workerInfo);
    }
  }

  private removeIdleWorker(workerInfo: WorkerInfo): void {
    const index = this.idleWorkers.indexOf(workerInfo);
    if (index >= 0) {
      this.idleWorkers.splice(index, 1);
    }
  }

  private logErrorMessage(error: LogDataInit, exitAfter: boolean = false): void {
    const logData = new LogData(error);
    if (exitAfter) {
      logger.printErrorAndExit(logData);
    } else {
      logger.printError(logData);
    }
  }

  private logErrorMessages(error: LogDataInit | LogDataInit[]): void {
    const errors = Array.isArray(error) ? error : [error];
    errors.forEach((err: LogDataInit) => {
      this.logErrorMessage(err);
    });
  }
}

function isWorkerMessage(message: unknown): message is WorkerMessage {
  if (!isRecord(message) || typeof message.type !== 'string') {
    return false;
  }
  if (message.type === WorkerMessageType.TASK_FINISHED) {
    return (
      message.data === undefined ||
      (isRecord(message.data) && (message.data.taskId === undefined || typeof message.data.taskId === 'string'))
    );
  }
  if (!isRecord(message.data)) {
    return false;
  }
  switch (message.type) {
    case WorkerMessageType.DECL_GENERATED:
      return typeof message.data.taskId === 'string';
    case WorkerMessageType.ERROR_OCCURED:
      return typeof message.data.taskId === 'string' && isLogDataOrArray(message.data.error);
    case WorkerMessageType.LOG:
      if (!Object.values(LogLevel).includes(message.data.level as LogLevel)) {
        return false;
      }
      return message.data.level === LogLevel.ERROR || message.data.level === LogLevel.ERROR_AND_EXIT
        ? isLogData(message.data.error)
        : typeof message.data.message === 'string';
    default:
      return false;
  }
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null;
}

function isLogDataOrArray(value: unknown): boolean {
  return Array.isArray(value) ? value.every(isLogData) : isLogData(value);
}

function isLogData(value: unknown): boolean {
  return isRecord(value) && typeof value.code === 'string' && typeof value.description === 'string';
}
