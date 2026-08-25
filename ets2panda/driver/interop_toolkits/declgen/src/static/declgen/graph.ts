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

/** A node in the declgen task dependency graph; `predecessors`/`descendants` hold neighbor ids. */
export class GraphNode<T> {
  public readonly id: string;
  public readonly data: T;
  public readonly predecessors = new Set<string>();
  public readonly descendants = new Set<string>();

  public constructor(id: string, data: T) {
    this.id = id;
    this.data = data;
  }
}

/** Minimal dependency graph used by TaskManager to dispatch declgen tasks in topological order. */
export class Graph<T> {
  private readonly id2Node = new Map<string, GraphNode<T>>();
  public readonly nodes = new Set<GraphNode<T>>();

  public static createGraphFromNodes<T>(nodes: readonly GraphNode<T>[]): Graph<T> {
    const graph = new Graph<T>();

    for (const node of nodes) {
      if (graph.id2Node.has(node.id)) {
        throw new Error(`Corrupted declgen task graph: duplicate node id ${node.id}`);
      }
      const copy = new GraphNode(node.id, node.data);
      graph.id2Node.set(copy.id, copy);
      graph.nodes.add(copy);
    }

    for (const node of nodes) {
      const copy = graph.getNodeById(node.id);
      for (const predecessor of node.predecessors) {
        graph.addEdge(predecessor, copy.id);
      }
      for (const descendant of node.descendants) {
        graph.addEdge(copy.id, descendant);
      }
    }

    graph.verifyAcyclic();
    return graph;
  }

  public hasNodes(): boolean {
    return this.nodes.size > 0;
  }

  public getNodeById(id: string): GraphNode<T> {
    const node = this.id2Node.get(id);
    if (node === undefined) {
      throw new Error(`Corrupted declgen task graph: unknown node id ${id}`);
    }
    return node;
  }

  public addNode(node: GraphNode<T>): void {
    if (this.id2Node.has(node.id)) {
      throw new Error(`Corrupted declgen task graph: duplicate node id ${node.id}`);
    }

    const predecessors = [...node.predecessors];
    const descendants = [...node.descendants];
    const copy = new GraphNode(node.id, node.data);
    this.nodes.add(copy);
    this.id2Node.set(copy.id, copy);

    try {
      for (const predecessor of predecessors) {
        this.addEdge(predecessor, copy.id);
      }
      for (const descendant of descendants) {
        this.addEdge(copy.id, descendant);
      }
      this.verifyAcyclic();
    } catch (error) {
      for (const predecessor of copy.predecessors) {
        this.getNodeById(predecessor).descendants.delete(copy.id);
      }
      for (const descendant of copy.descendants) {
        this.getNodeById(descendant).predecessors.delete(copy.id);
      }
      this.nodes.delete(copy);
      this.id2Node.delete(copy.id);
      throw error;
    }
  }

  public verifyAcyclic(): void {
    const remainingPredecessors = new Map<string, number>();
    const ready: GraphNode<T>[] = [];

    for (const node of this.nodes) {
      for (const predecessor of node.predecessors) {
        if (!this.getNodeById(predecessor).descendants.has(node.id)) {
          throw new Error(`Corrupted declgen task graph: inconsistent edge ${predecessor} -> ${node.id}`);
        }
      }
      for (const descendant of node.descendants) {
        if (!this.getNodeById(descendant).predecessors.has(node.id)) {
          throw new Error(`Corrupted declgen task graph: inconsistent edge ${node.id} -> ${descendant}`);
        }
      }
      remainingPredecessors.set(node.id, node.predecessors.size);
      if (node.predecessors.size === 0) {
        ready.push(node);
      }
    }

    let visited = 0;
    while (ready.length > 0) {
      const node = ready.shift()!;
      visited++;
      for (const descendant of node.descendants) {
        const remaining = remainingPredecessors.get(descendant)! - 1;
        remainingPredecessors.set(descendant, remaining);
        if (remaining === 0) {
          ready.push(this.getNodeById(descendant));
        }
      }
    }

    if (visited !== this.nodes.size) {
      throw new Error('Corrupted declgen task graph: dependency cycle detected');
    }
  }

  private addEdge(predecessorId: string, descendantId: string): void {
    const predecessor = this.getNodeById(predecessorId);
    const descendant = this.getNodeById(descendantId);
    predecessor.descendants.add(descendantId);
    descendant.predecessors.add(predecessorId);
  }
}
