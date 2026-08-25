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

import type * as context from '../context';
import * as common from '@interop-toolkits/common';
import type { DependencyNode } from './graph';
import type { PartialResolver, PartialResolvedDependencyMap } from './partialResolver';
import { DependencyGraph } from './graph';

/**
 * Orchestrates the dynamic and static resolvers into one merged graph.
 *
 * Whole-project ("full") strategy: every file of each language is handed to its
 * resolver in a single pass, so every cross-language reference (sentinel) is
 * guaranteed to have a matching real node produced by the other resolver. No
 * fixed-point iteration is needed — resolve each side once, then merge once.
 */
export class CrossLanguageResolver {
  constructor(
    public readonly context: context.Context,
    private readonly dynamicResolver: PartialResolver,
    private readonly staticResolver: PartialResolver,
  ) {
    dynamicResolver.setContext(context);
    staticResolver.setContext(context);
  }

  resolve(): DependencyGraph {
    const dynamicPartial = this.dynamicResolver.resolve([...this.context.fileManager.dynamicSourceFiles]);
    const staticPartial = this.staticResolver.resolve([...this.context.fileManager.staticSourceFiles]);

    const merged = new Map<string, DependencyNode>();
    this.mergeInto(merged, dynamicPartial);
    this.mergeInto(merged, staticPartial);
    this.populateDependants(merged);

    return new DependencyGraph(merged);
  }

  /**
   * Merge a partial result into the global node map.
   * - Language and sentinel provenance are independent node properties.
   * - Sentinel and resolved states are preserved when either side contributes them.
   * - Dependency edges are unioned.
   */
  private mergeInto(nodes: Map<string, DependencyNode>, partial: PartialResolvedDependencyMap): void {
    for (const [key, node] of partial.nodes) {
      const existing = nodes.get(key);
      if (!existing) {
        nodes.set(key, { ...node, dependencies: [...node.dependencies], dependants: [] });
        continue;
      }
      if (existing.type !== node.type) {
        throw new common.errors.InternalError(`Conflicting dependency node types for ${key}`);
      }
      existing.isSentinel ||= node.isSentinel;
      existing.isResolved ||= node.isResolved;
      existing.dependencies = unionKeys(existing.dependencies, node.dependencies);
    }
  }

  private populateDependants(nodes: ReadonlyMap<string, DependencyNode>): void {
    for (const node of nodes.values()) {
      node.dependants = [];
    }
    for (const node of nodes.values()) {
      for (const dependency of node.dependencies) {
        const dependencyNode = nodes.get(dependency);
        if (dependencyNode && !dependencyNode.dependants.includes(node.fileName)) {
          dependencyNode.dependants.push(node.fileName);
        }
      }
    }
  }
}

function unionKeys(a: string[], b: string[]): string[] {
  return [...new Set([...a, ...b])];
}
