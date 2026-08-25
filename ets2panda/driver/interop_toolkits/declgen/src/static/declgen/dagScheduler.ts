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

import type { Graph, GraphNode } from './graph';

export type TaskState = 'blocked' | 'ready' | 'running' | 'succeeded' | 'failed' | 'skipped';

export class DagScheduler<T> {
  private readonly states = new Map<string, TaskState>();
  private readonly remainingBlockers = new Map<string, number>();
  private readonly readyQueue: string[] = [];
  private terminalTasks = 0;

  public constructor(
    private readonly graph: Graph<T>,
    private readonly descendantsFirst: boolean,
  ) {
    graph.verifyAcyclic();
    for (const node of graph.nodes) {
      const blockerCount = this.blockers(node).size;
      this.remainingBlockers.set(node.id, blockerCount);
      if (blockerCount === 0) {
        this.states.set(node.id, 'ready');
        this.readyQueue.push(node.id);
      } else {
        this.states.set(node.id, 'blocked');
      }
    }
  }

  public takeNext(): GraphNode<T> | undefined {
    while (this.readyQueue.length > 0) {
      const id = this.readyQueue.shift()!;
      if (this.states.get(id) !== 'ready') {
        continue;
      }
      this.states.set(id, 'running');
      return this.graph.getNodeById(id);
    }
    return undefined;
  }

  public complete(taskId: string, success: boolean): void {
    if (this.states.get(taskId) !== 'running') {
      throw new Error(`Cannot complete declgen task ${taskId}: task is not running`);
    }

    this.states.set(taskId, success ? 'succeeded' : 'failed');
    this.terminalTasks++;
    if (success) {
      this.unlockNext(taskId);
    } else {
      this.skipDependents(taskId);
    }
  }

  public isComplete(): boolean {
    return this.terminalTasks === this.graph.nodes.size;
  }

  public isSuccessful(): boolean {
    return this.isComplete() && [...this.states.values()].every((state) => state === 'succeeded');
  }

  public getState(taskId: string): TaskState | undefined {
    return this.states.get(taskId);
  }

  private unlockNext(taskId: string): void {
    for (const nextId of this.dependents(this.graph.getNodeById(taskId))) {
      if (this.states.get(nextId) !== 'blocked') {
        continue;
      }
      const remaining = this.remainingBlockers.get(nextId)! - 1;
      this.remainingBlockers.set(nextId, remaining);
      if (remaining === 0) {
        this.states.set(nextId, 'ready');
        this.readyQueue.push(nextId);
      }
    }
  }

  private skipDependents(taskId: string): void {
    const pending = [...this.dependents(this.graph.getNodeById(taskId))];
    while (pending.length > 0) {
      const dependentId = pending.pop()!;
      const state = this.states.get(dependentId);
      if (state !== 'blocked' && state !== 'ready') {
        continue;
      }
      this.states.set(dependentId, 'skipped');
      this.terminalTasks++;
      pending.push(...this.dependents(this.graph.getNodeById(dependentId)));
    }
  }

  private blockers(node: GraphNode<T>): ReadonlySet<string> {
    return this.descendantsFirst ? node.descendants : node.predecessors;
  }

  private dependents(node: GraphNode<T>): ReadonlySet<string> {
    return this.descendantsFirst ? node.predecessors : node.descendants;
  }
}
