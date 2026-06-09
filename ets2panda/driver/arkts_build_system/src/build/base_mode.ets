/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import { Ets2panda } from '../util/ets2panda';
import {
  MERGED_ABC_FILE,
} from '../pre_define';
import {
  BuildConfig,
  ModuleInfo,
  CompileJobInfo,
  FileInfo,
  JobContentType,
} from '../types';
import {
  Logger,
} from '../logger';
import { DependencyAnalyzer } from '../dependency_analyzer';
import {
  ArkTSConfigGenerator,
} from './generate_arktsconfig';
import { Graph, GraphNode } from '../util/graph';

export class BaseMode {
  protected buildConfig: BuildConfig;
  protected entryFiles: Set<string>;
  protected fileToModule: Map<string, ModuleInfo>;
  protected moduleInfos: Map<string, ModuleInfo>;
  protected abcFiles: Set<string>;
  protected mergedAbcFile: string;
  protected logger: Logger;
  protected arktsConfigGenerator: ArkTSConfigGenerator;

  constructor(buildConfig: BuildConfig) {
    this.buildConfig = buildConfig;
    this.entryFiles = new Set<string>(buildConfig.compileFiles);
    this.fileToModule = new Map<string, ModuleInfo>();
    this.moduleInfos = new Map<string, ModuleInfo>();
    this.abcFiles = new Set<string>();
    this.mergedAbcFile = joinPath(this.outputDir, MERGED_ABC_FILE);
    this.logger = Logger.getInstance();
    this.arktsConfigGenerator = new ArkTSConfigGenerator(buildConfig);
    this.processBuildConfig();
  }

  public run(): void {
    const outputs: string[] = [];
    const graph = this.getDependencyGraph(outputs);
    this.runSequential(graph);
  }

  protected processBuildConfig(): void {
    const mainModule = this.createMainModuleInfo();
    this.moduleInfos.set(mainModule.packageName, mainModule);
    for (const file of this.entryFiles) {
      this.fileToModule.set(file, mainModule);
    }
    this.arktsConfigGenerator.generateArkTSConfigFile(mainModule, this.buildConfig.enableDeclgenEts2Ts);
  }

  protected getDependencyGraph(outputs: string[]): Graph<CompileJobInfo> {
    const dependencyAnalyzer = new DependencyAnalyzer(this.buildConfig, this.arktsConfigGenerator, false);
    return dependencyAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, outputs);
  }

  protected runSequential(graph: Graph<CompileJobInfo>): void {
    const ets2panda = Ets2panda.getInstance(this.buildConfig);
    ets2panda.initialize();
    try {
      const orderedIds = Graph.topologicalSort(graph);
      for (const nodeId of orderedIds) {
        const node = graph.getNodeById(nodeId);
        ets2panda.compile(nodeId, node.data);
        this.collectOutputs(node.data);
      }
    } finally {
      ets2panda.finalize();
    }
  }

  protected collectOutputs(job: CompileJobInfo): void {
    if (job.contentType === JobContentType.FILE) {
      this.abcFiles.add((job.content as FileInfo).output);
      return;
    }
    for (const fileInfo of job.content as FileInfo[]) {
      this.abcFiles.add(fileInfo.output);
    }
  }

  protected createMainModuleInfo(): ModuleInfo {
    return {
      isMainModule: true,
      packageName: this.buildConfig.packageName,
      moduleRootPath: this.buildConfig.moduleRootPath,
      moduleType: this.buildConfig.moduleType.valueOf(),
      sourceRoots: this.buildConfig.sourceRoots,
      entryFile: this.buildConfig.entryFile,
      arktsConfigFile: joinPath(this.cacheDir, 'arktsconfig.json'),
      declgenV1OutPath: this.buildConfig.declgenV1OutPath,
      declgenV2OutPath: this.buildConfig.declgenV2OutPath,
      declgenBridgeCodePath: this.buildConfig.declgenBridgeCodePath,
      dependencies: [],
      staticDependencyModules: new Map<string, ModuleInfo>(),
      dynamicDependencyModules: new Map<string, ModuleInfo>(),
      language: undefined,
      declFilesPath: undefined,
      abcPath: undefined,
      frameworkMode: this.buildConfig.frameworkMode,
      useEmptyPackage: this.buildConfig.useEmptyPackage,
      byteCodeHar: this.buildConfig.byteCodeHar,
      staticFiles: this.buildConfig.compileFiles,
      moduleName: undefined,
      bundleType: this.buildConfig.bundleType,
      bundleName: this.buildConfig.bundleName,
      packageVersion: undefined,
      originalPackageNameMap: undefined,
    } as ModuleInfo;
  }

  protected get outputDir(): string {
    return this.buildConfig.loaderOutPath;
  }

  protected get cacheDir(): string {
    return this.buildConfig.cachePath;
  }
}

function joinPath(lhs: string, rhs: string): string {
  if (lhs.endsWith('/')) {
    return lhs + rhs;
  }
  return lhs + '/' + rhs;
}
