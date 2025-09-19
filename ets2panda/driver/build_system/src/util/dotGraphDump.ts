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
import { JobInfo } from '../types';


function formLabelForNode(job: JobInfo): string {
    let res: string = `{ <target> ${job.id}`
    for (const file of job.fileList) {
        res += ` | ${file}`
    }
    res += '}'
    return res;
}

function createNodes(graph: dot.Digraph, jobs: Record<string, JobInfo>) {
    for (const [jobId, jobInfo] of Object.entries(jobs)) {
        const node = new dot.Node(jobId, {
            [dot.attribute.shape]: 'record',
            [dot.attribute.label]: formLabelForNode(jobInfo),
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
            const edge = new dot.Edge([node.port('target'), dependantNode.port('target')], {})
            graph.addEdge(edge)
        }
    }
}

export function dotGraphDump(jobs: Record<string, JobInfo>): string {
    const G = new dot.Digraph({ rankdir : 'BT' })

    createNodes(G, jobs);
    connectNodes(G, jobs);

    return dot.toDot(G);
}
