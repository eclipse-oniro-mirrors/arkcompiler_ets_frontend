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

import type { Context } from '../../src/context';
import { CrossLanguageResolver } from '../../src/resolver';
import type { PartialResolver } from '../../src/resolver';
import { NodeType } from '../../src/resolver/graph';
import type { DependencyNode, PartialResolvedDependencyMap } from '../../src/resolver';

class FakeResolver {
  public entries: string[] = [];

  public constructor(private readonly nodes: Map<string, DependencyNode>) {}

  public setContext(_context: Context): void {}

  public resolve(entries: string[]): PartialResolvedDependencyMap {
    this.entries = entries;
    return { nodes: this.nodes, sentinels: [] };
  }
}

describe('CrossLanguageResolver', () => {
  it('resolves all module source files and builds direct dependant edges', () => {
    const staticEntry = '/project/static-entry.ets';
    const staticDependency = '/project/static-dependency.ets';
    const dynamicEntry = '/project/dynamic-entry.ts';
    const staticResolver = new FakeResolver(
      new Map([
        [staticEntry, node(staticEntry, NodeType.STATIC, [staticDependency])],
        [staticDependency, node(staticDependency, NodeType.STATIC, [])],
      ]),
    );
    const dynamicResolver = new FakeResolver(
      new Map([[dynamicEntry, node(dynamicEntry, NodeType.DYNAMIC, [staticEntry])]]),
    );
    const resolver = new CrossLanguageResolver(
      contextWithFiles([staticEntry, staticDependency], [dynamicEntry]),
      dynamicResolver as unknown as PartialResolver,
      staticResolver as unknown as PartialResolver,
    );

    const graph = resolver.resolve();

    expect(staticResolver.entries).toEqual([staticEntry, staticDependency]);
    expect(dynamicResolver.entries).toEqual([dynamicEntry]);
    expect(graph.getDependants(staticDependency)).toEqual([staticEntry]);
    expect(graph.getDependants(staticEntry)).toEqual([dynamicEntry]);
    expect(graph.getDependantChain(staticDependency)).toEqual([staticEntry, dynamicEntry]);
    expect(graph.getDependencyChain(dynamicEntry)).toEqual([staticEntry, staticDependency]);
  });

  it('preserves sentinel provenance after merging real nodes in either order', () => {
    const staticFile = '/project/static.ets';
    const dynamicFile = '/project/dynamic.ts';
    const dynamicResolver = new FakeResolver(
      new Map([
        [staticFile, node(staticFile, NodeType.STATIC, [], true, false)],
        [dynamicFile, node(dynamicFile, NodeType.DYNAMIC, [], false, true)],
      ]),
    );
    const staticResolver = new FakeResolver(
      new Map([
        [staticFile, node(staticFile, NodeType.STATIC, [], false, true)],
        [dynamicFile, node(dynamicFile, NodeType.DYNAMIC, [], true, false)],
      ]),
    );
    const resolver = new CrossLanguageResolver(
      contextWithFiles([staticFile], [dynamicFile]),
      dynamicResolver as unknown as PartialResolver,
      staticResolver as unknown as PartialResolver,
    );

    const graph = resolver.resolve();

    expect(graph.getNode(staticFile)).toMatchObject({
      type: NodeType.STATIC,
      isSentinel: true,
      isResolved: true,
    });
    expect(graph.getNode(dynamicFile)).toMatchObject({
      type: NodeType.DYNAMIC,
      isSentinel: true,
      isResolved: true,
    });
    expect(graph.getSentinels()).toEqual([
      expect.objectContaining({ fileName: staticFile }),
      expect.objectContaining({ fileName: dynamicFile }),
    ]);
  });

  it('returns a same-language dependency closure without traversing cross-language dependencies', () => {
    const dynamicA = '/project/a.ts';
    const dynamicB = '/project/b.ts';
    const dynamicD = '/project/d.ts';
    const dynamicF = '/project/f.ts';
    const staticC = '/project/c.ets';
    const staticE = '/project/e.ets';
    const dynamicResolver = new FakeResolver(
      new Map([
        [dynamicA, node(dynamicA, NodeType.DYNAMIC, [dynamicB, staticC])],
        [dynamicB, node(dynamicB, NodeType.DYNAMIC, [dynamicD])],
        [dynamicD, node(dynamicD, NodeType.DYNAMIC, [])],
        [dynamicF, node(dynamicF, NodeType.DYNAMIC, [])],
      ]),
    );
    const staticResolver = new FakeResolver(
      new Map([
        [staticC, node(staticC, NodeType.STATIC, [staticE, dynamicF])],
        [staticE, node(staticE, NodeType.STATIC, [])],
      ]),
    );
    const resolver = new CrossLanguageResolver(
      contextWithFiles([staticC, staticE], [dynamicA, dynamicB, dynamicD, dynamicF]),
      dynamicResolver as unknown as PartialResolver,
      staticResolver as unknown as PartialResolver,
    );

    const graph = resolver.resolve();

    expect(graph.getPartialDependencyChain(dynamicA)).toEqual([dynamicB, dynamicD]);
  });

  it('rejects a partial dependency chain containing a dependency without a node', () => {
    const dynamicA = '/project/a.ts';
    const missingDynamicDependency = '/project/missing.ts';
    const dynamicResolver = new FakeResolver(
      new Map([[dynamicA, node(dynamicA, NodeType.DYNAMIC, [missingDynamicDependency])]]),
    );
    const staticResolver = new FakeResolver(new Map());
    const resolver = new CrossLanguageResolver(
      contextWithFiles([], [dynamicA]),
      dynamicResolver as unknown as PartialResolver,
      staticResolver as unknown as PartialResolver,
    );

    const graph = resolver.resolve();

    expect(() => graph.getPartialDependencyChain(dynamicA)).toThrow(
      `Dependency node not found for ${missingDynamicDependency}`,
    );
  });

  it('stops dependency-chain traversal at a node rejected by a predicate', () => {
    const dynamicA = '/project/a.ts';
    const dynamicSource = '/project/source.ts';
    const dynamicNonSource = '/project/sdk.d.ts';
    const dynamicLeaf = '/project/leaf.ts';
    const dynamicResolver = new FakeResolver(
      new Map([
        [dynamicA, node(dynamicA, NodeType.DYNAMIC, [dynamicSource])],
        [dynamicSource, node(dynamicSource, NodeType.DYNAMIC, [dynamicNonSource])],
        [dynamicNonSource, node(dynamicNonSource, NodeType.DYNAMIC, [dynamicLeaf])],
        [dynamicLeaf, node(dynamicLeaf, NodeType.DYNAMIC, [])],
      ]),
    );
    const staticResolver = new FakeResolver(new Map());
    const resolver = new CrossLanguageResolver(
      contextWithFiles([], [dynamicA, dynamicSource, dynamicNonSource, dynamicLeaf]),
      dynamicResolver as unknown as PartialResolver,
      staticResolver as unknown as PartialResolver,
    );
    const graph = resolver.resolve();
    const shouldTraverse = (node: DependencyNode): boolean => node.fileName !== dynamicNonSource;

    expect(graph.getDependencyChain(dynamicA, shouldTraverse)).toEqual([dynamicSource]);
    expect(graph.getPartialDependencyChain(dynamicA, shouldTraverse)).toEqual([dynamicSource]);
  });
});

function contextWithFiles(staticSourceFiles: string[], dynamicSourceFiles: string[]): Context {
  return {
    fileManager: {
      staticSourceFiles: new Set(staticSourceFiles),
      dynamicSourceFiles: new Set(dynamicSourceFiles),
    },
    cachePath: '',
  } as unknown as Context;
}

function node(
  fileName: string,
  type: NodeType,
  dependencies: string[],
  isSentinel = false,
  isResolved = true,
): DependencyNode {
  return { fileName, type, isSentinel, isResolved, dependencies, dependants: [] };
}
