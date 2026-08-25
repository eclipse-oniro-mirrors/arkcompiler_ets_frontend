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

import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

import { Graph, GraphNode } from './graph';

const DEFAULT_CLUSTER_FILES_THRESHOLD = 460;

export interface DependencyFileMap {
  dependants: Record<string, string[]>;
  dependencies: Record<string, string[]>;
  outputMatching: Record<string, string>;
}

export interface Cluster {
  inputFiles: string[];
}

export interface DepAnalyzerOptions {
  clusterFilesThreshold?: number;
}

/** Builds a cluster-only dependency graph for the supplied ArkTS entry files. */
export class DepAnalyzer {
  private readonly entryFiles: string[];
  private readonly arktsconfigPath: string;
  private readonly dependencyAnalyzerPath: string;
  private readonly clusterFilesThreshold: number;

  public constructor(
    entryFiles: readonly string[],
    arktsconfigPath: string,
    dependencyAnalyzerPath: string,
    options: DepAnalyzerOptions = {},
  ) {
    this.entryFiles = [...new Set(entryFiles)];
    this.arktsconfigPath = arktsconfigPath;
    this.dependencyAnalyzerPath = dependencyAnalyzerPath;
    this.clusterFilesThreshold = options.clusterFilesThreshold ?? DEFAULT_CLUSTER_FILES_THRESHOLD;

    if (!Number.isInteger(this.clusterFilesThreshold) || this.clusterFilesThreshold < 1) {
      throw new RangeError(`clusterFilesThreshold must be a positive integer, got ${this.clusterFilesThreshold}`);
    }
  }

  public getGraph(): Graph<Cluster> {
    if (this.entryFiles.length === 0) {
      return new Graph<Cluster>();
    }

    return this.createClusterGraph(this.generateDependencyMap());
  }

  private generateDependencyMap(): DependencyFileMap {
    const workDir = mkdtempSync(join(tmpdir(), 'declgen-dep-analyzer-'));
    const inputPath = join(workDir, 'dependencyFileInfo.txt');
    const outputPath = join(workDir, 'dependency.json');

    try {
      writeFileSync(inputPath, this.entryFiles.join('\n'));
      execFileSync(
        this.dependencyAnalyzerPath,
        [`@${inputPath}`, `--arktsconfig=${this.arktsconfigPath}`, `--output=${outputPath}`],
        { stdio: 'pipe', encoding: 'utf-8' },
      );
      return this.filterDependencyMap(JSON.parse(readFileSync(outputPath, 'utf-8')) as DependencyFileMap);
    } finally {
      rmSync(workDir, { recursive: true, force: true });
    }
  }

  private filterDependencyMap(dependencyMap: DependencyFileMap): DependencyFileMap {
    const entryFileSet = new Set(this.entryFiles);
    const dependencies: Record<string, string[]> = {};
    const dependants: Record<string, string[]> = {};

    for (const file of this.entryFiles) {
      dependencies[file] = (dependencyMap.dependencies[file] ?? []).filter((dependency) =>
        entryFileSet.has(dependency),
      );
      dependants[file] = (dependencyMap.dependants[file] ?? []).filter((dependant) => entryFileSet.has(dependant));
    }

    return { dependencies, dependants, outputMatching: dependencyMap.outputMatching ?? {} };
  }

  private createClusterGraph(dependencyMap: DependencyFileMap): Graph<Cluster> {
    const components = this.findStronglyConnectedComponents(dependencyMap.dependencies);
    const componentByFile = new Map<string, number>();
    components.forEach((component, index) => component.forEach((file) => componentByFile.set(file, index)));

    const componentDependencies = components.map(() => new Set<number>());
    for (const [file, dependencies] of Object.entries(dependencyMap.dependencies)) {
      const component = componentByFile.get(file)!;
      for (const dependency of dependencies) {
        const dependencyComponent = componentByFile.get(dependency)!;
        if (component !== dependencyComponent) {
          componentDependencies[component].add(dependencyComponent);
        }
      }
    }

    const orderedComponents = this.topologicalOrder(componentDependencies);
    const clusters: string[][] = [];
    let currentCluster: string[] = [];
    for (const componentIndex of orderedComponents) {
      const component = components[componentIndex];
      if (currentCluster.length > 0 && currentCluster.length + component.length > this.clusterFilesThreshold) {
        clusters.push(currentCluster);
        currentCluster = [];
      }
      currentCluster.push(...component);
    }
    if (currentCluster.length > 0) {
      clusters.push(currentCluster);
    }

    return this.buildGraph(clusters, dependencyMap.dependencies);
  }

  private findStronglyConnectedComponents(dependencies: Record<string, string[]>): string[][] {
    let nextIndex = 0;
    const indexes = new Map<string, number>();
    const lowLinks = new Map<string, number>();
    const stack: string[] = [];
    const onStack = new Set<string>();
    const components: string[][] = [];

    const visit = (file: string): void => {
      indexes.set(file, nextIndex);
      lowLinks.set(file, nextIndex++);
      stack.push(file);
      onStack.add(file);

      for (const dependency of dependencies[file] ?? []) {
        if (!indexes.has(dependency)) {
          visit(dependency);
          lowLinks.set(file, Math.min(lowLinks.get(file)!, lowLinks.get(dependency)!));
        } else if (onStack.has(dependency)) {
          lowLinks.set(file, Math.min(lowLinks.get(file)!, indexes.get(dependency)!));
        }
      }

      if (lowLinks.get(file) !== indexes.get(file)) {
        return;
      }
      const component: string[] = [];
      let member: string;
      do {
        member = stack.pop()!;
        onStack.delete(member);
        component.push(member);
      } while (member !== file);
      components.push(component);
    };

    this.entryFiles.forEach((file) => {
      if (!indexes.has(file)) {
        visit(file);
      }
    });
    return components;
  }

  private topologicalOrder(componentDependencies: readonly Set<number>[]): number[] {
    const visited = new Set<number>();
    const order: number[] = [];
    const visit = (component: number): void => {
      if (visited.has(component)) {
        return;
      }
      visited.add(component);
      componentDependencies[component].forEach(visit);
      order.push(component);
    };
    componentDependencies.forEach((_dependencies, component) => visit(component));
    return order;
  }

  private buildGraph(clusters: string[][], dependencies: Record<string, string[]>): Graph<Cluster> {
    const clusterByFile = new Map<string, number>();
    clusters.forEach((cluster, index) => cluster.forEach((file) => clusterByFile.set(file, index)));
    const nodes = clusters.map((inputFiles, index) => new GraphNode<Cluster>(`cluster-${index}`, { inputFiles }));

    for (const [file, fileDependencies] of Object.entries(dependencies)) {
      const node = nodes[clusterByFile.get(file)!];
      for (const dependency of fileDependencies) {
        const dependencyNode = nodes[clusterByFile.get(dependency)!];
        if (node.id !== dependencyNode.id) {
          node.predecessors.add(dependencyNode.id);
          dependencyNode.descendants.add(node.id);
        }
      }
    }
    return Graph.createGraphFromNodes(nodes);
  }
}
