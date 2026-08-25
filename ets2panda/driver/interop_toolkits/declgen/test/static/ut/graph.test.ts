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

import { Graph, GraphNode } from '../../../src/static/declgen/graph';

describe('Graph', () => {
  test('constructs edges regardless of node order', () => {
    const predecessor = new GraphNode('predecessor', 1);
    const descendant = new GraphNode('descendant', 2);
    predecessor.descendants.add(descendant.id);
    descendant.predecessors.add(predecessor.id);

    const graph = Graph.createGraphFromNodes([descendant, predecessor]);

    expect(graph.getNodeById(predecessor.id).descendants).toEqual(new Set([descendant.id]));
    expect(graph.getNodeById(descendant.id).predecessors).toEqual(new Set([predecessor.id]));
  });

  test('rejects duplicate ids', () => {
    expect(() => Graph.createGraphFromNodes([new GraphNode('same', 1), new GraphNode('same', 2)])).toThrow(
      'duplicate node id same',
    );
  });

  test('rejects cycles', () => {
    const first = new GraphNode('first', 1);
    const second = new GraphNode('second', 2);
    first.descendants.add(second.id);
    second.descendants.add(first.id);

    expect(() => Graph.createGraphFromNodes([first, second])).toThrow('dependency cycle detected');
  });

  test('rejects references to unknown nodes', () => {
    const node = new GraphNode('node', 1);
    node.descendants.add('missing');

    expect(() => Graph.createGraphFromNodes([node])).toThrow('unknown node id missing');
  });
});
