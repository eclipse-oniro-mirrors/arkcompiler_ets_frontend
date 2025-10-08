/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from './error';
import { computeHash } from './utils';

export class GraphNode<T> {
    public id: string = '';
    public data: T;
    public predecessors: Set<string> = new Set<string>();
    public descendants: Set<string> = new Set<string>();

    constructor(id: string, data: T) {
        this.id = id;
        this.data = data;
    }
}

export class Graph<T> {
    private id2Node: Map<string, GraphNode<T>> = new Map<string, GraphNode<T>>();
    private adjacency: Record<string, Set<string>> = {};
    // NOTE: should be private
    public nodes: Set<GraphNode<T>> = new Set<GraphNode<T>>();

    public hasNodes() {
        return this.nodes.size > 0;
    }

    public getNodeById(id: string): GraphNode<T> {
        return this.id2Node.get(id)!;
    }

    static createGraphFromNodes<T>(nodes: GraphNode<T>[]) {
        const res: Graph<T> = new Graph<T>();

        for (const node of nodes) {
            res.id2Node.set(node.id, node);
            res.nodes.add(node);
            for (const predecessor of node.predecessors) {
                if (!res.adjacency[predecessor]) {
                    res.adjacency[predecessor] = new Set<string>();
                }
                res.adjacency[predecessor].add(node.id);
            }
            res.adjacency[node.id] = new Set<string>();
            for (const descendant of node.descendants) {
                res.adjacency[node.id].add(descendant);
            }
        }
        res.verify();
        return res;
    }

    public verify() {
        if (this.nodes.size != this.id2Node.size) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                    `Corrupted graph`
                )
            )
        }
        for (const node of this.nodes) {
            if (!this.id2Node.has(node.id)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                        `Corrupted graph node: non-existent node ${node.id}`
                    )
                )
            }
            for (const desc of node.descendants) {
                if (!this.id2Node.has(desc)) {
                    throw new DriverError(
                        LogDataFactory.newInstance(
                            ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                            `Corrupted graph node: non-existent descendant ${desc} for node ${node.id}`
                        )
                    )
                }
            }
            for (const pred of node.predecessors) {
                if (!this.id2Node.has(pred)) {
                    throw new DriverError(
                        LogDataFactory.newInstance(
                            ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                            `Corrupted graph node: non-existent predecessor ${pred} for node ${node.id}`
                        )
                    )
                }
            }
        }

        for (const [nodeId, neighbors] of Object.entries(this.adjacency)) {
            if (!this.id2Node.has(nodeId)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                        `Corrupted adjacency matrix`
                    )
                )
            }
            const node = this.getNodeById(nodeId);
            for (const neighbor of neighbors) {
                if (!this.id2Node.has(neighbor)) {
                    throw new DriverError(
                        LogDataFactory.newInstance(
                            ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                            `Corrupted adjacency matrix`
                        )
                    )
                }
                const neighborNode = this.getNodeById(neighbor);
                if (!node.descendants.has(neighbor) || !neighborNode.predecessors.has(nodeId)) {
                    throw new DriverError(
                        LogDataFactory.newInstance(
                            ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                            `Corrupted adjacency matrix: inconsistent edge beetween ${nodeId} and ${neighbor}`
                        )
                    )
                }
            }
        }
    }

    filter(predicate: (node: GraphNode<T>) => boolean): Graph<T> {
        const nodes = Array.from(this.nodes);

        for (const node of nodes) {
            if (!predicate(node)) {
                this.removeNode(node);
            }
        }
        return this;
    }

    find(predicate: (node: GraphNode<T>) => boolean): GraphNode<T> | undefined {
        for (const node of this.nodes) {
            if (predicate(node)) {
                return node;
            }
        }
        return undefined;
    }

    addNode(node: GraphNode<T>) {
        this.nodes.add(node);
        this.id2Node.set(node.id, node);
        for (const predecessor of node.predecessors) {
            if (!this.id2Node.has(predecessor)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                        `Wrong predecessor ${predecessor} for node ${node.id}`
                    )
                )
            }
            this.getNodeById(predecessor).descendants.add(node.id);
            this.adjacency[predecessor].add(node.id);
        }
        this.adjacency[node.id] = new Set<string>();
        for (const descendant of node.descendants) {
            if (!this.id2Node.has(descendant)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                        `Wrong descendant ${descendant} for node ${node.id}`
                    )
                )
            }
            this.getNodeById(descendant).predecessors.add(node.id);
            this.adjacency[node.id].add(descendant);
        }
    }

    removeNode(node: GraphNode<T>) {
        this.nodes.delete(node);
        this.id2Node.delete(node.id)
        delete this.adjacency[node.id];
        for (const neighbors of Object.values(this.adjacency)) {
            neighbors.delete(node.id)
        }

        for (const pred of node.predecessors) {
            this.getNodeById(pred).descendants.delete(node.id);
        }
        for (const desc of node.descendants) {
            this.getNodeById(desc).predecessors.delete(node.id);
        }
    }

    mergeNodes(lhs: GraphNode<T>, rhs: GraphNode<T>, mergeDataCb: (lhs: GraphNode<T>, rhs: GraphNode<T>) => T): GraphNode<T> {
        const resId = computeHash([lhs.id, rhs.id].join('|'));
        const resData = mergeDataCb(lhs, rhs);

        const resNode = new GraphNode<T>(resId, resData);

        resNode.predecessors = new Set([...lhs.predecessors, ...rhs.predecessors]
            .filter((node) => (node != lhs.id) && (node != rhs.id)));
        resNode.descendants = new Set([...lhs.descendants, ...rhs.descendants]
            .filter((node) => (node != lhs.id) && (node != rhs.id)));

        this.removeNode(lhs);
        this.removeNode(rhs);
        this.addNode(resNode);
        return resNode;
    }

    static dfs(root: string, adj: Record<string, Set<string>>, visited: Set<string>, order: string[]): void {
        visited.add(root);
        for (const neighbor of adj[root]) {
            if (!visited.has(neighbor)) {
                Graph.dfs(neighbor, adj, visited, order);
            }
        }
        order.push(root);
    }

    static findStronglyConnectedComponents<T>(graph: Graph<T>): Map<string, Set<string>> {
        const visited = new Set<string>();
        const order: string[] = [];

        graph.nodes.forEach(node => {
            if (!visited.has(node.id)) {
                Graph.dfs(node.id, graph.adjacency, visited, order);
            }
        });

        visited.clear();
        order.reverse();

        const reversedAdj: Record<string, Set<string>> = {};
        for (const node of graph.nodes) {
            for (const neighbor of graph.adjacency[node.id]) {
                if (!reversedAdj[neighbor]) {
                    reversedAdj[neighbor] = new Set<string>();
                }
                reversedAdj[neighbor].add(node.id);
            }
            if (!reversedAdj[node.id]) {
                reversedAdj[node.id] = new Set<string>();
            }
        }

        const components = new Map<string, Set<string>>();
        for (const nodeId of order) {
            if (!visited.has(nodeId)) {
                const component = new Set<string>();
                const componentOrder: string[] = [];
                Graph.dfs(nodeId, reversedAdj, visited, componentOrder);
                if (componentOrder.length > 1) {
                    componentOrder.map((nodeId: string) => component.add(nodeId));
                    const componentId = computeHash(componentOrder.sort().join('|'));
                    components.set(componentId, component);
                }
            }
        }
        return components;
    }

    static topologicalSort<T>(graph: Graph<T>): string[] {
        const visited = new Set<string>();
        const order: string[] = [];

        graph.nodes.forEach(node => {
            if (!visited.has(node.id)) {
                Graph.dfs(node.id, graph.adjacency, visited, order);
            }
        });

        order.reverse();
        return order;
    }

    static collapseCycles<T>(graph: Graph<T>, mergeDataCb: (srcData: GraphNode<T>, dstData: GraphNode<T>) => T): Graph<T> {
        const scc: Map<string, Set<string>> = Graph.findStronglyConnectedComponents(graph);
        if (scc.size == 0) {
            return graph;
        }

        for (const [_, nodesSet] of scc.entries()) {
            const nodesArr = Array.from(nodesSet)
            let cycleNode: GraphNode<T> = graph.getNodeById(nodesArr.shift()!);
            while (nodesArr.length > 0) {
                const toBeMergedNode: GraphNode<T> = graph.getNodeById(nodesArr.shift()!);
                cycleNode = graph.mergeNodes(cycleNode, toBeMergedNode, mergeDataCb);
            }
        }

        return graph;
    }
}
