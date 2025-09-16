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
import { JobInfo, ModuleInfo } from '../types';

function formLabelForNode(job: JobInfo, fileToModule: Map<string, ModuleInfo>): string {
    let res: string = `{ id: ${job.id.slice(0, 5)}`
    for (const file of job.fileList) {
        const module = fileToModule.get(file)!
        res += ` | ${path.join(module.packageName, path.relative(module.moduleRootPath, file))}`
    }
    res += '}'
    return res;
}

function createNodes(graph: dot.Digraph, jobs: Record<string, JobInfo>, fileToModule: Map<string, ModuleInfo>) {
    for (const [jobId, jobInfo] of Object.entries(jobs)) {
        const node = new dot.Node(jobId, {
            [dot.attribute.shape]: 'record',
            [dot.attribute.label]: formLabelForNode(jobInfo, fileToModule),
            [dot.attribute.style]: 'filled',
            [dot.attribute.fillcolor]: jobInfo.fileList.length > 1 ? 'lightcoral' : undefined
        })
        node
        graph.addNode(node)
    }
}

function connectNodes(graph: dot.Digraph, jobs: Record<string, JobInfo>) {
    for (const [job, jobInfo] of Object.entries(jobs)) {
        const node = graph.getNode(job)!
        for (const dependant of jobInfo.jobDependants) {
            const dependantNode = graph.getNode(dependant)!
            const edge = new dot.Edge([dependantNode, node], {})
            graph.addEdge(edge)
        }
    }
}

export function dotGraphDump(jobs: Record<string, JobInfo>, fileToModule: Map<string, ModuleInfo>): string {
    const G = new dot.Digraph({ rankdir : 'TB' })

    createNodes(G, jobs, fileToModule);
    connectNodes(G, jobs);

    return dot.toDot(G);
}
