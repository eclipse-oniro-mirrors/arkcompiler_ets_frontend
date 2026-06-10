import { Graph, GraphNode } from '../../ets_src/util/graph';
import { assertEquals } from './assertions';

function main(): void {
  let harB: GraphNode<string> = new GraphNode<string>('harB', 'harB');
  let harA: GraphNode<string> = new GraphNode<string>('harA', 'harA');
  harA.predecessors.add('harB');
  harB.descendants.add('harA');
  let graph: Graph<string> = Graph.createGraphFromNodes<string>([harA, harB]);
  let order: string[] = Graph.topologicalSort<string>(graph);
  assertEquals(order.join(','), 'harB,harA');
  console.log(order.join(','));
  console.log('graph-ok');
}
