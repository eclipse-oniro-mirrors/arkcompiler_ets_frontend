import { Graph, GraphNode } from '../../ets_src/util/graph';
import { DriverError, ErrorCode } from '../../ets_src/util/error';
import { changeDeclgenFileExtension, changeFileExtension, computeHash, dirname, extname, isSubPathOf, joinPath, normalizePath, resolvePath, substituteEnvVarsInString } from '../../ets_src/util/utils';

function assertTrue(value: boolean, message: string): void {
  if (!value) {
    throw new Error(message);
  }
}

function assertEquals(actual: string, expected: string): void {
  if (actual !== expected) {
    throw new Error('Expected ' + expected + ', got ' + actual);
  }
}

function testTopologicalSort(): void {
  let harB: GraphNode<string> = new GraphNode<string>('harB', 'harB');
  let harA: GraphNode<string> = new GraphNode<string>('harA', 'harA');
  harA.predecessors.add('harB');
  harB.descendants.add('harA');
  let graph: Graph<string> = Graph.createGraphFromNodes<string>([harA, harB]);
  let order: string[] = Graph.topologicalSort<string>(graph);
  assertEquals(order.join(','), 'harB,harA');
}

function testCycleError(): void {
  let a: GraphNode<string> = new GraphNode<string>('a', 'a');
  let b: GraphNode<string> = new GraphNode<string>('b', 'b');
  a.predecessors.add('b');
  a.descendants.add('b');
  b.predecessors.add('a');
  b.descendants.add('a');
  let graph: Graph<string> = Graph.createGraphFromNodes<string>([a, b]);
  try {
    Graph.topologicalSort<string>(graph);
  } catch (error) {
    let driverError: DriverError = error as DriverError;
    assertEquals(driverError.logData.code, ErrorCode.BUILDSYSTEM_GRAPH_ERROR);
    return;
  }
  throw new Error('Expected DriverError');
}

function testUtils(): void {
  assertEquals(normalizePath('/a//b/../c'), '/a/c');
  assertEquals(joinPath('/a/', 'b', '../c'), '/a/c');
  assertEquals(resolvePath('/a/b', '../c'), '/a/c');
  assertEquals(dirname('/a/b/c.ets'), '/a/b');
  assertEquals(extname('/a/b/c.d.ets'), '.d.ets');
  assertEquals(changeFileExtension('/a/b/c.ets', '.abc'), '/a/b/c.abc');
  assertEquals(changeDeclgenFileExtension('/a/b/c.d.ets', '.ts'), '/a/b/c.ts');
  assertTrue(isSubPathOf('/a/b/c', '/a/b'), 'subpath true');
  assertTrue(!isSubPathOf('/a/bc', '/a/b'), 'subpath false');
  assertEquals(computeHash('same'), computeHash('same'));
  assertEquals(substituteEnvVarsInString('/sdk/${ROOT}/ets', (name: string): string => name === 'ROOT' ? 'mock' : ''), '/sdk/mock/ets');
}

function main(): void {
  testTopologicalSort();
  testCycleError();
  testUtils();
  console.log('harB,harA');
}
