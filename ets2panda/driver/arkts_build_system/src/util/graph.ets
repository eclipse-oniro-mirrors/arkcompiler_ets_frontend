/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
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
  private adjacency: Map<string, Set<string>> = new Map<string, Set<string>>();
  public nodes: Set<GraphNode<T>> = new Set<GraphNode<T>>();

  public hasNodes(): boolean {
    return this.nodes.size > 0;
  }

  public getNodeById(id: string): GraphNode<T> {
    const node = this.id2Node.get(id);
    if (node === undefined) {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Graph node does not exist: ' + id));
    }
    return node;
  }

  static createGraphFromNodes<T>(nodes: GraphNode<T>[]): Graph<T> {
    const res: Graph<T> = new Graph<T>();
    for (const node of nodes) {
      res.id2Node.set(node.id, node);
      res.nodes.add(node);
      for (const predecessor of node.predecessors) {
        let neighbors = res.adjacency.get(predecessor);
        if (neighbors === undefined) {
          neighbors = new Set<string>();
          res.adjacency.set(predecessor, neighbors);
        }
        neighbors.add(node.id);
      }
      if (!res.adjacency.has(node.id)) {
        res.adjacency.set(node.id, new Set<string>());
      }
      const nodeNeighbors = res.adjacency.get(node.id) as Set<string>;
      for (const descendant of node.descendants) {
        nodeNeighbors.add(descendant);
      }
    }
    res.verify();
    return res;
  }

  public verify(): void {
    if (this.nodes.size !== this.id2Node.size) {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Corrupted graph'));
    }
    for (const node of this.nodes) {
      if (!this.id2Node.has(node.id)) {
        throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Corrupted graph node: non-existent node ' + node.id));
      }
      for (const desc of node.descendants) {
        if (!this.id2Node.has(desc)) {
          throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Corrupted graph node: non-existent descendant ' + desc + ' for node ' + node.id));
        }
      }
      for (const pred of node.predecessors) {
        if (!this.id2Node.has(pred)) {
          throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Corrupted graph node: non-existent predecessor ' + pred + ' for node ' + node.id));
        }
      }
    }
  }

  public filter(predicate: (node: GraphNode<T>) => boolean): Graph<T> {
    const nodes = Array.from(this.nodes);
    for (const node of nodes) {
      if (!predicate(node)) {
        this.removeNode(node);
      }
    }
    return this;
  }

  public find(predicate: (node: GraphNode<T>) => boolean): GraphNode<T> | undefined {
    for (const node of this.nodes) {
      if (predicate(node)) {
        return node;
      }
    }
    return undefined;
  }

  public addNode(node: GraphNode<T>): void {
    this.nodes.add(node);
    this.id2Node.set(node.id, node);
    for (const predecessor of node.predecessors) {
      if (!this.id2Node.has(predecessor)) {
        throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Wrong predecessor ' + predecessor + ' for node ' + node.id));
      }
      this.getNodeById(predecessor).descendants.add(node.id);
      let neighbors = this.adjacency.get(predecessor);
      if (neighbors === undefined) {
        neighbors = new Set<string>();
        this.adjacency.set(predecessor, neighbors);
      }
      neighbors.add(node.id);
    }
    const descendants = new Set<string>();
    this.adjacency.set(node.id, descendants);
    for (const descendant of node.descendants) {
      if (!this.id2Node.has(descendant)) {
        throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Wrong descendant ' + descendant + ' for node ' + node.id));
      }
      this.getNodeById(descendant).predecessors.add(node.id);
      descendants.add(descendant);
    }
  }

  public removeNode(node: GraphNode<T>): void {
    this.nodes.delete(node);
    this.id2Node.delete(node.id);
    this.adjacency.delete(node.id);
    for (const neighbors of this.adjacency.values()) {
      neighbors.delete(node.id);
    }
    for (const pred of node.predecessors) {
      if (this.id2Node.has(pred)) {
        this.getNodeById(pred).descendants.delete(node.id);
      }
    }
    for (const desc of node.descendants) {
      if (this.id2Node.has(desc)) {
        this.getNodeById(desc).predecessors.delete(node.id);
      }
    }
  }

  public mergeNodes(lhs: GraphNode<T>, rhs: GraphNode<T>, mergeDataCb: (lhs: GraphNode<T>, rhs: GraphNode<T>) => T): GraphNode<T> {
    const resId = computeHash(lhs.id + '|' + rhs.id);
    const resData = mergeDataCb(lhs, rhs);
    const resNode = new GraphNode<T>(resId, resData);
    for (const pred of lhs.predecessors) {
      if (pred !== lhs.id && pred !== rhs.id) {
        resNode.predecessors.add(pred);
      }
    }
    for (const pred of rhs.predecessors) {
      if (pred !== lhs.id && pred !== rhs.id) {
        resNode.predecessors.add(pred);
      }
    }
    for (const desc of lhs.descendants) {
      if (desc !== lhs.id && desc !== rhs.id) {
        resNode.descendants.add(desc);
      }
    }
    for (const desc of rhs.descendants) {
      if (desc !== lhs.id && desc !== rhs.id) {
        resNode.descendants.add(desc);
      }
    }
    this.removeNode(lhs);
    this.removeNode(rhs);
    this.addNode(resNode);
    return resNode;
  }

  static dfs(root: string, adj: Map<string, Set<string>>, visited: Set<string>, order: string[]): void {
    visited.add(root);
    const neighbors = adj.get(root);
    if (neighbors !== undefined) {
      for (const neighbor of neighbors) {
        if (!visited.has(neighbor)) {
          Graph.dfs(neighbor, adj, visited, order);
        }
      }
    }
    order.push(root);
  }

  static findStronglyConnectedComponents<T>(graph: Graph<T>): Map<string, Set<string>> {
    const visited = new Set<string>();
    const order: string[] = [];
    for (const node of graph.nodes) {
      if (!visited.has(node.id)) {
        Graph.dfs(node.id, graph.adjacency, visited, order);
      }
    }
    visited.clear();
    order.reverse();
    const reversedAdj: Map<string, Set<string>> = new Map<string, Set<string>>();
    for (const node of graph.nodes) {
      const neighbors = graph.adjacency.get(node.id);
      if (neighbors !== undefined) {
        for (const neighbor of neighbors) {
          let reversedNeighbors = reversedAdj.get(neighbor);
          if (reversedNeighbors === undefined) {
            reversedNeighbors = new Set<string>();
            reversedAdj.set(neighbor, reversedNeighbors);
          }
          reversedNeighbors.add(node.id);
        }
      }
      if (!reversedAdj.has(node.id)) {
        reversedAdj.set(node.id, new Set<string>());
      }
    }
    const components = new Map<string, Set<string>>();
    for (const nodeId of order) {
      if (!visited.has(nodeId)) {
        const component = new Set<string>();
        const componentOrder: string[] = [];
        Graph.dfs(nodeId, reversedAdj, visited, componentOrder);
        if (componentOrder.length > 1) {
          for (const componentNodeId of componentOrder) {
            component.add(componentNodeId);
          }
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
    for (const node of graph.nodes) {
      if (!visited.has(node.id)) {
        Graph.dfs(node.id, graph.adjacency, visited, order);
      }
    }
    order.reverse();
    return order;
  }

  static collapseCycles<T>(graph: Graph<T>, mergeDataCb: (srcData: GraphNode<T>, dstData: GraphNode<T>) => T): Graph<T> {
    const scc: Map<string, Set<string>> = Graph.findStronglyConnectedComponents(graph);
    if (scc.size === 0) {
      return graph;
    }
    for (const nodesSet of scc.values()) {
      const nodesArr = Array.from(nodesSet);
      const first = nodesArr.shift();
      if (first === undefined) {
        continue;
      }
      let cycleNode: GraphNode<T> = graph.getNodeById(first);
      while (nodesArr.length > 0) {
        const next = nodesArr.shift();
        if (next !== undefined) {
          cycleNode = graph.mergeNodes(cycleNode, graph.getNodeById(next), mergeDataCb);
        }
      }
    }
    return graph;
  }
}
