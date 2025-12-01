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


import * as dot from 'ts-graphviz';
import * as path from 'path';
import { Graph, GraphNode } from '../util/graph';
import { JobInfo } from '../types';

function formLabelForNode(node: GraphNode<JobInfo>): string {
    const job = node.data;
    let res: string = `{ id: ${node.id.slice(0, 5)}`
    for (const file of job.fileList) {
        res += ` | ${path.join(job.fileInfo.moduleName, path.relative(job.fileInfo.moduleRoot, file))}`
    }
    res += '}'
    return res;
}

function createNodes(dorGraph: dot.Digraph, graph: Graph<JobInfo>): void {
    for (const node of graph.nodes) {
        const dotNode = new dot.Node(node.id, {
            [dot.attribute.shape]: 'record',
            [dot.attribute.label]: formLabelForNode(node),
            [dot.attribute.style]: 'filled',
            [dot.attribute.fillcolor]: node.data.fileList.length > 1 ? 'lightcoral' : undefined
        })
        dorGraph.addNode(dotNode)
    }
}

function connectNodes(dotGraph: dot.Digraph, graph: Graph<JobInfo>): void {
    for (const node of graph.nodes) {
        for (const dependant of node.descendants) {
            const dependantNode = dotGraph.getNode(dependant)!
            const edge = new dot.Edge([node , dependantNode], {})
            dotGraph.addEdge(edge)
        }
    }
}

export function dotGraphDump(graph: Graph<JobInfo>): string {
    const G = new dot.Digraph({ rankdir : 'TB' })

    createNodes(G, graph);
    connectNodes(G, graph);

    return dot.toDot(G);
}
