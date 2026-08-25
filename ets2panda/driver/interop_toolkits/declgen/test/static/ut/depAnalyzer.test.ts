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

import { DepAnalyzer } from '../../../src/static/declgen/depAnalyzer';
import type { Cluster, DependencyFileMap } from '../../../src/static/declgen/depAnalyzer';
import type { Graph } from '../../../src/static/declgen/graph';

interface DepAnalyzerInternals {
  createClusterGraph(dependencyMap: DependencyFileMap): Graph<Cluster>;
}

function createGraph(
  entryFiles: string[],
  dependencies: Record<string, string[]>,
  clusterFilesThreshold = 460,
): Graph<Cluster> {
  const dependants: Record<string, string[]> = Object.fromEntries(entryFiles.map((file) => [file, []]));
  for (const [file, fileDependencies] of Object.entries(dependencies)) {
    for (const dependency of fileDependencies) {
      dependants[dependency].push(file);
    }
  }
  const dependencyMap: DependencyFileMap = { dependencies, dependants, outputMatching: {} };
  const analyzer = new DepAnalyzer(entryFiles, '/project/arktsconfig.json', '/bin/dep-analyzer', {
    clusterFilesThreshold,
  });
  return (analyzer as unknown as DepAnalyzerInternals).createClusterGraph(dependencyMap);
}

describe('DepAnalyzer', () => {
  it('keeps a single file as a cluster', () => {
    const graph = createGraph(['/project/a.ets'], { '/project/a.ets': [] });

    expect(graph.nodes.size).toBe(1);
    expect([...graph.nodes][0].data).toEqual({ inputFiles: ['/project/a.ets'] });
  });

  it('keeps a dependency cycle in one cluster even when it exceeds the threshold', () => {
    const graph = createGraph(
      ['/project/a.ets', '/project/b.ets'],
      {
        '/project/a.ets': ['/project/b.ets'],
        '/project/b.ets': ['/project/a.ets'],
      },
      1,
    );

    expect(graph.nodes.size).toBe(1);
    expect(new Set([...graph.nodes][0].data.inputFiles)).toEqual(new Set(['/project/a.ets', '/project/b.ets']));
  });

  it('preserves dependencies between threshold-sized clusters', () => {
    const graph = createGraph(
      ['/project/a.ets', '/project/b.ets', '/project/c.ets'],
      {
        '/project/a.ets': [],
        '/project/b.ets': ['/project/a.ets'],
        '/project/c.ets': ['/project/b.ets'],
      },
      1,
    );
    const nodes = [...graph.nodes];

    expect(nodes.map((node) => node.data.inputFiles)).toEqual([
      ['/project/a.ets'],
      ['/project/b.ets'],
      ['/project/c.ets'],
    ]);
    expect(nodes[1].predecessors).toEqual(new Set([nodes[0].id]));
    expect(nodes[2].predecessors).toEqual(new Set([nodes[1].id]));
  });
});
