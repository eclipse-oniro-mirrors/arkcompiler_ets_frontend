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

import { LogData } from '@interop-toolkits/common';

import { Graph, GraphNode } from '../../../src/static/declgen/graph';
import { TaskManager, WorkerMessageType } from '../../../src/static/declgen/taskManager';
import type { DriverWorker, WorkerFactory, WorkerInfo } from '../../../src/static/declgen/taskManager';

type WorkerEvent = 'message' | 'exit' | 'error';
type Listener = (...args: never[]) => void;

class FakeWorker implements DriverWorker {
  private static nextId = 1;
  private readonly listeners = new Map<WorkerEvent, Listener[]>();
  public readonly id = FakeWorker.nextId++;
  public readonly sent: Array<{ type: string; data: unknown }> = [];
  public stopped = false;
  public sendError: Error | null = null;

  public constructor(private readonly instances: FakeWorker[]) {
    instances.push(this);
  }

  public on(msg: 'message', listener: (message: unknown) => void): DriverWorker;
  public on(msg: 'exit', listener: (code: number | null, signal: NodeJS.Signals | null) => void): DriverWorker;
  public on(msg: 'error', listener: (error: Error) => void): DriverWorker;
  public on(msg: WorkerEvent, listener: Listener): DriverWorker {
    const listeners = this.listeners.get(msg) ?? [];
    listeners.push(listener);
    this.listeners.set(msg, listeners);
    return this;
  }

  public send(type: string, data?: unknown, callback?: (error: Error | null) => void): void {
    this.sent.push({ type, data });
    callback?.(this.sendError);
  }

  public stop(): number {
    this.stopped = true;
    return 0;
  }

  public getId(): number {
    return this.id;
  }

  public getWorkerPath(): string {
    return 'fake-worker';
  }

  public spawnNewInstance(): DriverWorker {
    return new FakeWorker(this.instances);
  }

  public emitMessage(message: unknown): void {
    this.emit('message', message);
  }

  public emitExit(code: number | null = 1, signal: NodeJS.Signals | null = null): void {
    this.emit('exit', code, signal);
  }

  private emit(event: WorkerEvent, ...args: unknown[]): void {
    for (const listener of this.listeners.get(event) ?? []) {
      listener(...(args as never[]));
    }
  }
}

class FakeWorkerFactory implements WorkerFactory {
  public readonly instances: FakeWorker[] = [];

  public spawnWorker(): DriverWorker {
    return new FakeWorker(this.instances);
  }
}

function createManager(
  graph: Graph<string>,
  timeout = 1000,
): {
  manager: TaskManager<string>;
  factory: FakeWorkerFactory;
} {
  const manager = new TaskManager<string>(
    (workerInfo: WorkerInfo) =>
      new LogData({ code: '11430002', description: `Worker ${workerInfo.id} exited unexpectedly` }),
    1,
    timeout,
  );
  const factory = new FakeWorkerFactory();
  manager.startWorkers(factory);
  manager.buildGraph = graph;
  manager.initTaskQueue();
  return { manager, factory };
}

function taskId(worker: FakeWorker, sendIndex = 0): string {
  const data = worker.sent[sendIndex]!.data as { taskId: string };
  return data.taskId;
}

function finishTask(worker: FakeWorker, success: boolean): void {
  const id = taskId(worker, worker.sent.length - 1);
  worker.emitMessage(
    success
      ? { type: WorkerMessageType.DECL_GENERATED, data: { taskId: id } }
      : { type: WorkerMessageType.ERROR_OCCURED, data: { taskId: id, error: [] } },
  );
  worker.emitMessage({ type: WorkerMessageType.TASK_FINISHED, data: { taskId: id } });
}

describe('TaskManager', () => {
  afterEach(() => {
    jest.useRealTimers();
  });

  test('waits for TASK_FINISHED before settling a successful task', async () => {
    const graph = Graph.createGraphFromNodes([new GraphNode('task', 'payload')]);
    const { manager, factory } = createManager(graph);
    const completion = manager.finish();
    const worker = factory.instances[0]!;

    worker.emitMessage({ type: WorkerMessageType.DECL_GENERATED, data: { taskId: 'task' } });
    let resolved = false;
    void completion.then(() => {
      resolved = true;
    });
    await Promise.resolve();
    expect(resolved).toBe(false);

    worker.emitMessage({ type: WorkerMessageType.TASK_FINISHED, data: { taskId: 'task' } });
    await expect(completion).resolves.toBe(true);
  });

  test('times out after a result when TASK_FINISHED never arrives and ignores stale worker events', async () => {
    jest.useFakeTimers();
    const graph = Graph.createGraphFromNodes([new GraphNode('task', 'payload')]);
    const { manager, factory } = createManager(graph, 10);
    const completion = manager.finish();
    const oldWorker = factory.instances[0]!;

    oldWorker.emitMessage({ type: WorkerMessageType.DECL_GENERATED, data: { taskId: 'task' } });
    jest.advanceTimersByTime(10);

    const replacement = factory.instances[1]!;
    oldWorker.emitExit();
    expect(factory.instances).toHaveLength(2);
    expect(replacement.stopped).toBe(false);
    await expect(completion).resolves.toBe(false);
  });

  test('skips dependent tasks after failure and continues independent tasks', async () => {
    const dependent = new GraphNode('dependent', 'dependent');
    const failed = new GraphNode('failed', 'failed');
    const independent = new GraphNode('independent', 'independent');
    dependent.descendants.add(failed.id);
    const graph = Graph.createGraphFromNodes([dependent, failed, independent]);
    const { manager, factory } = createManager(graph);
    const completion = manager.finish();
    const worker = factory.instances[0]!;

    expect(taskId(worker)).toBe('failed');
    finishTask(worker, false);
    expect(taskId(worker, 1)).toBe('independent');
    finishTask(worker, true);

    expect(worker.sent.map((message) => (message.data as { taskId: string }).taskId)).not.toContain('dependent');
    await expect(completion).resolves.toBe(false);
  });

  test('replaces an idle worker that exits before dispatch', async () => {
    const graph = Graph.createGraphFromNodes([new GraphNode('task', 'payload')]);
    const { manager, factory } = createManager(graph);
    const oldWorker = factory.instances[0]!;
    oldWorker.emitExit();

    const completion = manager.finish();
    const replacement = factory.instances[1]!;
    expect(taskId(replacement)).toBe('task');
    finishTask(replacement, true);
    await expect(completion).resolves.toBe(true);
  });

  test('fails immediately when IPC send callback reports a closed channel', async () => {
    const graph = Graph.createGraphFromNodes([new GraphNode('task', 'payload')]);
    const { manager, factory } = createManager(graph);
    factory.instances[0]!.sendError = new Error('Channel closed');

    await expect(manager.finish()).resolves.toBe(false);
    expect(factory.instances).toHaveLength(2);
  });

  test('rejects invalid worker and timeout configuration', () => {
    const onExit = (): LogData => new LogData({ code: '11430002', description: 'exit' });
    expect(() => new TaskManager(onExit, 0)).toThrow('maxWorkers must be a positive integer');
    expect(() => new TaskManager(onExit, 1, 0)).toThrow('taskTimeoutMs must be positive');
  });
});
