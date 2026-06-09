import { LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from './error';
import { computeHash } from './utils';

export class GraphNode<T> {
  public id: string;
  public data: T;
  public predecessors: Set<string> = new Set<string>();
  public descendants: Set<string> = new Set<string>();

  public constructor(id: string, data: T) {
    this.id = id;
    this.data = data;
  }
}

export class Graph<T> {
  private id2Node: Map<string, GraphNode<T>> = new Map<string, GraphNode<T>>();
  private adjacency: Map<string, Set<string>> = new Map<string, Set<string>>();
  public nodes: GraphNode<T>[] = [];

  public hasNodes(): boolean {
    return this.nodes.length > 0;
  }

  public getNodeById(id: string): GraphNode<T> {
    let node: GraphNode<T> | undefined = this.id2Node.get(id);
    if (node === undefined) {
      this.throwGraphError('Corrupted graph node: non-existent node ' + id);
    }
    return node as GraphNode<T>;
  }

  public static createGraphFromNodes<T>(nodes: GraphNode<T>[]): Graph<T> {
    let graph: Graph<T> = new Graph<T>();
    for (let i: int = 0; i < nodes.length; i++) {
      graph.registerNode(nodes[i]);
    }
    for (let i: int = 0; i < nodes.length; i++) {
      let node: GraphNode<T> = nodes[i];
      node.predecessors.forEach((predecessor: string) => {
        graph.addAdjacency(predecessor, node.id);
      });
      node.descendants.forEach((descendant: string) => {
        graph.addAdjacency(node.id, descendant);
      });
    }
    graph.verify();
    return graph;
  }

  public verify(): void {
    if (this.nodes.length !== this.id2Node.size) {
      this.throwGraphError('Corrupted graph');
    }
    for (let i: int = 0; i < this.nodes.length; i++) {
      let node: GraphNode<T> = this.nodes[i];
      if (!this.id2Node.has(node.id)) {
        this.throwGraphError('Corrupted graph node: non-existent node ' + node.id);
      }
      if (!this.adjacency.has(node.id)) {
        this.throwGraphError('Corrupted adjacency matrix');
      }
      node.descendants.forEach((descendant: string) => {
        if (!this.id2Node.has(descendant)) {
          this.throwGraphError('Corrupted graph node: non-existent descendant ' + descendant + ' for node ' + node.id);
        }
      });
      node.predecessors.forEach((predecessor: string) => {
        if (!this.id2Node.has(predecessor)) {
          this.throwGraphError('Corrupted graph node: non-existent predecessor ' + predecessor + ' for node ' + node.id);
        }
      });
    }
    this.adjacency.forEach((neighbors: Set<string>, nodeId: string) => {
      if (!this.id2Node.has(nodeId)) {
        this.throwGraphError('Corrupted adjacency matrix');
      }
      let node: GraphNode<T> = this.getNodeById(nodeId);
      neighbors.forEach((neighbor: string) => {
        if (!this.id2Node.has(neighbor)) {
          this.throwGraphError('Corrupted adjacency matrix');
        }
        let neighborNode: GraphNode<T> = this.getNodeById(neighbor);
        if (!node.descendants.has(neighbor) || !neighborNode.predecessors.has(nodeId)) {
          this.throwGraphError('Corrupted adjacency matrix: inconsistent edge between ' + nodeId + ' and ' + neighbor);
        }
      });
    });
  }

  public filter(predicate: (node: GraphNode<T>) => boolean): Graph<T> {
    let snapshot: GraphNode<T>[] = this.nodes.slice();
    for (let i: int = 0; i < snapshot.length; i++) {
      if (!predicate(snapshot[i])) {
        this.removeNode(snapshot[i]);
      }
    }
    return this;
  }

  public find(predicate: (node: GraphNode<T>) => boolean): GraphNode<T> | undefined {
    for (let i: int = 0; i < this.nodes.length; i++) {
      if (predicate(this.nodes[i])) {
        return this.nodes[i];
      }
    }
    return undefined;
  }

  public addNode(node: GraphNode<T>): void {
    if (this.id2Node.has(node.id)) {
      this.throwGraphError('Duplicate graph node ' + node.id);
    }
    this.registerNode(node);
    node.predecessors.forEach((predecessor: string) => {
      if (!this.id2Node.has(predecessor)) {
        this.throwGraphError('Wrong predecessor ' + predecessor + ' for node ' + node.id);
      }
      this.getNodeById(predecessor).descendants.add(node.id);
      this.addAdjacency(predecessor, node.id);
    });
    node.descendants.forEach((descendant: string) => {
      if (!this.id2Node.has(descendant)) {
        this.throwGraphError('Wrong descendant ' + descendant + ' for node ' + node.id);
      }
      this.getNodeById(descendant).predecessors.add(node.id);
      this.addAdjacency(node.id, descendant);
    });
    this.verify();
  }

  public removeNode(node: GraphNode<T>): void {
    if (!this.id2Node.has(node.id)) {
      return;
    }
    this.id2Node.delete(node.id);
    this.adjacency.delete(node.id);
    for (let i: int = this.nodes.length - 1; i >= 0; i--) {
      if (this.nodes[i].id === node.id) {
        this.nodes.splice(i, 1);
      }
    }
    this.adjacency.forEach((neighbors: Set<string>) => {
      neighbors.delete(node.id);
    });
    node.predecessors.forEach((predecessor: string) => {
      if (this.id2Node.has(predecessor)) {
        this.getNodeById(predecessor).descendants.delete(node.id);
      }
    });
    node.descendants.forEach((descendant: string) => {
      if (this.id2Node.has(descendant)) {
        this.getNodeById(descendant).predecessors.delete(node.id);
      }
    });
  }

  public mergeNodes(lhs: GraphNode<T>, rhs: GraphNode<T>, mergeDataCb: (lhs: GraphNode<T>, rhs: GraphNode<T>) => T): GraphNode<T> {
    let resId: string = computeHash(lhs.id + '|' + rhs.id);
    let resNode: GraphNode<T> = new GraphNode<T>(resId, mergeDataCb(lhs, rhs));
    lhs.predecessors.forEach((id: string) => {
      if (id !== lhs.id && id !== rhs.id) {
        resNode.predecessors.add(id);
      }
    });
    rhs.predecessors.forEach((id: string) => {
      if (id !== lhs.id && id !== rhs.id) {
        resNode.predecessors.add(id);
      }
    });
    lhs.descendants.forEach((id: string) => {
      if (id !== lhs.id && id !== rhs.id) {
        resNode.descendants.add(id);
      }
    });
    rhs.descendants.forEach((id: string) => {
      if (id !== lhs.id && id !== rhs.id) {
        resNode.descendants.add(id);
      }
    });
    this.removeNode(lhs);
    this.removeNode(rhs);
    this.addNode(resNode);
    return resNode;
  }

  public static topologicalSort<T>(graph: Graph<T>): string[] {
    graph.verify();
    let inDegree: Map<string, int> = new Map<string, int>();
    let order: string[] = [];
    let queue: string[] = [];
    for (let i: int = 0; i < graph.nodes.length; i++) {
      let node: GraphNode<T> = graph.nodes[i];
      inDegree.set(node.id, node.predecessors.size as int);
      if (node.predecessors.size === 0) {
        queue.push(node.id);
      }
    }
    let head: int = 0;
    while (head < queue.length) {
      let id: string = queue[head];
      head++;
      order.push(id);
      let descendants: Set<string> | undefined = graph.adjacency.get(id);
      if (descendants === undefined) {
        graph.throwGraphError('Corrupted adjacency matrix');
      }
      (descendants as Set<string>).forEach((descendant: string) => {
        let value: int = (inDegree.get(descendant) as int) - 1;
        inDegree.set(descendant, value);
        if (value === 0) {
          queue.push(descendant);
        }
      });
    }
    if (order.length !== graph.nodes.length) {
      graph.throwGraphError('Cyclic dependencies detected');
    }
    return order;
  }

  public static collapseCycles<T>(graph: Graph<T>, mergeDataCb: (srcData: GraphNode<T>, dstData: GraphNode<T>) => T): Graph<T> {
    Graph.topologicalSort(graph);
    return graph;
  }

  private registerNode(node: GraphNode<T>): void {
    this.nodes.push(node);
    this.id2Node.set(node.id, node);
    if (!this.adjacency.has(node.id)) {
      this.adjacency.set(node.id, new Set<string>());
    }
  }

  private addAdjacency(from: string, to: string): void {
    if (!this.id2Node.has(from) || !this.id2Node.has(to)) {
      this.throwGraphError('Corrupted adjacency matrix');
    }
    let descendants: Set<string> | undefined = this.adjacency.get(from);
    if (descendants === undefined) {
      descendants = new Set<string>();
      this.adjacency.set(from, descendants as Set<string>);
    }
    (descendants as Set<string>).add(to);
    this.getNodeById(from).descendants.add(to);
    this.getNodeById(to).predecessors.add(from);
  }

  private throwGraphError(message: string): never {
    throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, message));
  }
}
