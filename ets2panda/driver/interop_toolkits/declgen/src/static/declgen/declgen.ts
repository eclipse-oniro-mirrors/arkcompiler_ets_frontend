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

import * as fs from 'node:fs';
import * as path from 'node:path';

import { LogData } from '@interop-toolkits/common';
import * as common from '@interop-toolkits/common';

import type { BuildConfig } from '../../buildConfig';
import { ErrorCode } from '../../errors';
import { logger } from '../../logger';
import { getDepAnalyzerPath } from '../../utils';
import { DepAnalyzer } from './depAnalyzer';
import { buildDeclgenOutputPath, Executor } from './executor';
import { Graph, GraphNode } from './graph';
import type { DeclgenTask } from './task';
import { DriverProcessFactory, TaskManager } from './taskManager';
import type { Task, WorkerInfo } from './taskManager';

const DECL_FILE_MAP_NAME = 'decl_file_map.json';
const FORCE_NO_PARALLEL_ENV = 'STATIC_DECLGEN_FORCE_NO_PARALLEL';

const STATIC_DECLGEN_CACHE_DIR = 'sta';
const ARKTSCONFIG_FILENAME = 'arktsconfig.json';

interface DeclFileInfo {
  delFilePath: string;
  declLastModified: number | null;
  sourceFilePath: string;
  sourceFileLastModified: number | null;
}

export class StaticDeclgen {
  private readonly fileManager: common.fileManager.FileManager;
  private readonly declFileMap = new Map<string, DeclFileInfo>();
  private buildGraph = new Graph<DeclgenTask>();
  private filesToGenerate: string[] = [];
  private readonly dependencyAnalyzerPath: string;
  private readonly cacheDir: string;

  public constructor(
    private readonly buildConfig: BuildConfig,
    private readonly inputFiles: string[],
    private readonly arktsconfigPath: string,
  ) {
    this.fileManager = new common.fileManager.FileManagerBuilder()
      .addDynamicSdkPaths(buildConfig.sdkPaths.dynamicSdkPaths)
      .addStaticSdkPaths(buildConfig.sdkPaths.staticSdkPaths)
      .addDynamicInteropSdkPaths(buildConfig.sdkPaths.dynamicInteropSdkPaths)
      .addStaticInteropSdkPaths(buildConfig.sdkPaths.staticInteropSdkPaths)
      .addModuleList(buildConfig.dependentModuleList)
      .build();

    this.dependencyAnalyzerPath = getDepAnalyzerPath(buildConfig.pandaSdkPath!);
    this.cacheDir = path.join(buildConfig.cachePath, STATIC_DECLGEN_CACHE_DIR);
  }

  public async run(): Promise<void> {
    this.loadDeclFileMap();
    const clusterGraph = new DepAnalyzer(this.inputFiles, this.arktsconfigPath, this.dependencyAnalyzerPath).getGraph();
    this.buildGraph = this.createIncrementalGraph(clusterGraph);

    if (!this.buildGraph.hasNodes()) {
      logger.printInfo('All declaration files are up to date');
      return;
    }

    await this.backupDeclgenFiles();

    if (this.shouldRunSerial()) {
      this.runSerial();
    } else {
      await this.runParallel();
    }

    await this.updateDeclFileMap();
    await this.saveDeclFileMap();
  }

  private shouldRunSerial(): boolean {
    return process.env[FORCE_NO_PARALLEL_ENV] === 'true';
  }

  private async runParallel(): Promise<void> {
    const taskManager = new TaskManager<DeclgenTask>(
      (workerInfo: WorkerInfo, task: Task<DeclgenTask>, code: number | null, signal: NodeJS.Signals | null) =>
        new LogData({
          code: ErrorCode.STATIC_WORKER_PROCESS_FAILED,
          description: `Worker ${workerInfo.id} exited while running task ${task.id}`,
          cause: `code=${String(code)}, signal=${String(signal)}`,
        }),
    );
    const workerFactory = new DriverProcessFactory(
      path.resolve(__dirname, 'worker.js'),
      [`declgen worker: ${__filename}`],
      { stdio: ['ignore', 'ignore', 'ignore', 'ipc'] },
    );

    taskManager.buildGraph = this.buildGraph;
    taskManager.startWorkers(workerFactory);
    taskManager.initTaskQueue();
    if (!(await taskManager.finish())) {
      throw new Error('One or more declaration generation tasks failed');
    }
  }

  private runSerial(): void {
    const executor = new Executor(this.buildConfig);
    for (const node of this.topologicalOrder(this.buildGraph)) {
      executor.execute(node.data.inputFiles, node.data.arktsconfigPath);
    }
  }

  private createIncrementalGraph(clusterGraph: Graph<{ inputFiles: string[] }>): Graph<DeclgenTask> {
    const retainedNodes: GraphNode<DeclgenTask>[] = [];
    const retainedNodeIds = new Set<string>();
    this.filesToGenerate = [];

    for (const node of clusterGraph.nodes) {
      const inputFiles = node.data.inputFiles.filter((file) => this.needsRegeneration(file));
      if (inputFiles.length === 0) {
        continue;
      }
      retainedNodeIds.add(node.id);
      this.filesToGenerate.push(...inputFiles);
      const taskNode = new GraphNode<DeclgenTask>(node.id, {
        inputFiles,
        arktsconfigPath: this.arktsconfigPath,
        buildConfig: this.buildConfig,
      });
      node.predecessors.forEach((id) => taskNode.predecessors.add(id));
      node.descendants.forEach((id) => taskNode.descendants.add(id));
      retainedNodes.push(taskNode);
    }

    for (const node of retainedNodes) {
      for (const predecessor of node.predecessors) {
        if (!retainedNodeIds.has(predecessor)) {
          node.predecessors.delete(predecessor);
        }
      }
      for (const descendant of node.descendants) {
        if (!retainedNodeIds.has(descendant)) {
          node.descendants.delete(descendant);
        }
      }
    }
    return Graph.createGraphFromNodes(retainedNodes);
  }

  private needsRegeneration(sourceFilePath: string): boolean {
    const fileInfo = this.declFileMap.get(sourceFilePath);
    if (!fileInfo || fileInfo.sourceFileLastModified === null || !fs.existsSync(fileInfo.delFilePath)) {
      return true;
    }
    return fs.statSync(sourceFilePath).mtimeMs > fileInfo.sourceFileLastModified;
  }

  private async backupDeclgenFiles(): Promise<void> {
    await Promise.all(
      this.filesToGenerate.map(async (file) => {
        const outputPath = this.getDeclOutputPath(file);
        const fileInfo = this.declFileMap.get(file);
        if (!fileInfo || fileInfo.declLastModified === null || !fs.existsSync(outputPath)) {
          return;
        }
        if ((await fs.promises.stat(outputPath)).mtimeMs > fileInfo.declLastModified) {
          await fs.promises.copyFile(outputPath, `${outputPath}.backup`);
          logger.printDebug(`Backed up declaration file: ${outputPath}.backup`);
        }
      }),
    );
  }

  private async updateDeclFileMap(): Promise<void> {
    await Promise.all(
      this.filesToGenerate.map(async (file) => {
        const declFilePath = this.getDeclOutputPath(file);
        const [sourceStat, declStat] = await Promise.all([fs.promises.stat(file), fs.promises.stat(declFilePath)]);
        this.declFileMap.set(file, {
          delFilePath: declFilePath,
          declLastModified: declStat.mtimeMs,
          sourceFilePath: file,
          sourceFileLastModified: sourceStat.mtimeMs,
        });
      }),
    );
  }

  private getDeclOutputPath(file: string): string {
    const moduleInfo = this.fileManager.queryFileMeta(file)?.module;
    if (moduleInfo === undefined) {
      throw new Error(`Cannot resolve module info for declgen input file: ${file}`);
    }
    return buildDeclgenOutputPath(file, moduleInfo, this.buildConfig.cachePath).declEtsOutputPath;
  }

  private loadDeclFileMap(): void {
    const declMapFile = path.join(this.buildConfig.cachePath, DECL_FILE_MAP_NAME);
    if (!fs.existsSync(declMapFile)) {
      return;
    }
    const data = JSON.parse(fs.readFileSync(declMapFile, 'utf-8')) as Record<string, DeclFileInfo>;
    Object.entries(data).forEach(([file, info]) => this.declFileMap.set(file, info));
  }

  private async saveDeclFileMap(): Promise<void> {
    const declMapFile = path.join(this.buildConfig.cachePath, DECL_FILE_MAP_NAME);
    const data = Object.fromEntries(this.declFileMap);
    await fs.promises.mkdir(path.dirname(declMapFile), { recursive: true });
    await fs.promises.writeFile(declMapFile, JSON.stringify(data, null, 2));
  }

  private topologicalOrder<T>(graph: Graph<T>): GraphNode<T>[] {
    const predecessorCounts = new Map<string, number>();
    const ready: GraphNode<T>[] = [];
    for (const node of graph.nodes) {
      predecessorCounts.set(node.id, node.predecessors.size);
      if (node.predecessors.size === 0) {
        ready.push(node);
      }
    }

    const result: GraphNode<T>[] = [];
    while (ready.length > 0) {
      const node = ready.shift()!;
      result.push(node);
      for (const descendantId of node.descendants) {
        const count = predecessorCounts.get(descendantId)! - 1;
        predecessorCounts.set(descendantId, count);
        if (count === 0) {
          ready.push(graph.getNodeById(descendantId));
        }
      }
    }
    return result;
  }
}
