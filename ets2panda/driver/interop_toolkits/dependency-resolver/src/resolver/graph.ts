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

import * as common from '@interop-toolkits/common';

export enum NodeType {
  STATIC,
  DYNAMIC,
}

export interface DependencyNode {
  /** Normalized absolute path; also the map key. */
  fileName: string;
  /** The language of the referenced file. */
  type: NodeType;
  /** Whether this file was discovered as a cross-language sentinel. Preserved after merging. */
  isSentinel: boolean;
  /** Whether a resolver for this file's own language contributed a real node. */
  isResolved: boolean;
  /** Normalized paths of direct dependencies (edges out of this node). */
  dependencies: string[];
  /** Normalized paths of direct dependants (edges into this node). */
  dependants: string[];
}

/**
 * The fully merged, cross-language dependency graph.
 *
 * Sentinel provenance is preserved after real nodes are merged. A sentinel is
 * unresolved only when `isSentinel` is true and `isResolved` is false.
 * Supports fast lookup of a file's direct dependencies and its full transitive
 * dependency chain.
 */
export class DependencyGraph {
  constructor(private readonly nodesByKey: Map<string, DependencyNode>) {}

  /** All nodes in the graph, keyed by normalized path. */
  get nodes(): ReadonlyMap<string, DependencyNode> {
    return this.nodesByKey;
  }

  getNode(fileName: string): DependencyNode | undefined {
    return this.nodesByKey.get(common.fileUtils.normalizePath(fileName));
  }

  /** All nodes discovered as cross-language sentinels. */
  getSentinels(): DependencyNode[] {
    return [...this.nodesByKey.values()].filter((node) => node.isSentinel);
  }

  /** Direct dependencies (one hop) of the given file. */
  getDependencies(fileName: string): string[] {
    return this.getNode(fileName)?.dependencies ?? [];
  }

  /** Direct dependants (one hop) of the given file. */
  getDependants(fileName: string): string[] {
    return this.getNode(fileName)?.dependants ?? [];
  }

  /**
   * Transitive dependency chain (closure) reachable from the file, excluding
   * the file itself. Cycle-safe; returned in deterministic DFS pre-order.
   */
  getDependencyChain(fileName: string, shouldTraverse?: (node: DependencyNode) => boolean): string[] {
    return this.getChain(fileName, (node) => node.dependencies, shouldTraverse);
  }

  /**
   * Transitive dependencies in the starting file's language, excluding the
   * file itself. Cross-language dependencies are excluded and terminate that
   * branch of traversal.
   */
  getPartialDependencyChain(fileName: string, shouldTraverse?: (node: DependencyNode) => boolean): string[] {
    const startNode = this.getNode(fileName);
    if (!startNode) {
      return [];
    }
    return this.getChain(
      fileName,
      (node) => node.dependencies,
      (node) => node.type === startNode.type && (shouldTraverse?.(node) ?? true),
      true,
    );
  }

  /** Transitive dependant chain (reverse closure), excluding the file itself. */
  getDependantChain(fileName: string): string[] {
    return this.getChain(fileName, (node) => node.dependants);
  }

  private getChain(
    fileName: string,
    getNeighbors: (node: DependencyNode) => readonly string[],
    shouldTraverse = (_node: DependencyNode): boolean => true,
    requireNodes = false,
  ): string[] {
    const start = common.fileUtils.normalizePath(fileName);
    const visited = new Set<string>([start]);
    const chain: string[] = [];
    const stack: string[] = [...(this.nodesByKey.get(start) ? getNeighbors(this.nodesByKey.get(start)!) : [])];
    while (stack.length > 0) {
      const key = stack.shift()!;
      if (visited.has(key)) {
        continue;
      }
      const node = this.nodesByKey.get(key);
      if (!node && requireNodes) {
        throw new common.errors.InternalError(`Dependency node not found for ${key}`);
      }
      if (node && !shouldTraverse(node)) {
        continue;
      }
      visited.add(key);
      chain.push(key);
      if (node) {
        stack.unshift(...getNeighbors(node));
      }
    }
    return chain;
  }
}
