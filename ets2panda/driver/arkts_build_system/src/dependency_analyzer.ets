/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import {
  ABC_SUFFIX,
  ETSCACHE_SUFFIX,
  ENABLE_CLUSTERS,
  ENABLE_DECL_FILE_CACHE,
} from './pre_define';
import {
  BuildConfig,
  ModuleInfo,
  CompileJobInfo,
  CompileJobType,
  JobContentType,
  JobInfo,
  FileInfo,
  isHarOrHsp,
  OHOS_MODULE_TYPE,
} from './types';
import {
  Logger,
  LogDataFactory
} from './logger';
import { ErrorCode, DriverError } from './util/error';
import { ArkTSConfigGenerator } from './build/generate_arktsconfig';
import { computeHash } from './util/utils';
import { Graph, GraphNode } from './util/graph';

export interface DependencyFileMap {
  dependants: Map<string, string[]>;
  dependencies: Map<string, string[]>;
  outputMatching: Map<string, string>;
}

export class DependencyAnalyzer {
  private logger: Logger;
  private mainModuleType: OHOS_MODULE_TYPE;
  private clusteredBuild: boolean;

  constructor(buildConfig: BuildConfig, generator: ArkTSConfigGenerator, clusteredBuild: boolean = ENABLE_CLUSTERS) {
    this.logger = Logger.getInstance();
    this.mainModuleType = buildConfig.moduleType;
    this.clusteredBuild = clusteredBuild;
  }

  public getGraph(entryFiles: Set<string>, fileToModule: Map<string, ModuleInfo>, moduleInfos: Map<string, ModuleInfo>,
    outputs: string[]): Graph<CompileJobInfo> {
    const dependencyGraph = this.createDependencyGraph(entryFiles, fileToModule, this.generateDependencyMap(entryFiles));
    dependencyGraph.verify();

    for (const node of dependencyGraph.nodes) {
      outputs.push((node.data.content as FileInfo).output);
    }

    return dependencyGraph;
  }

  private generateDependencyMap(entryFiles: Set<string>): DependencyFileMap {
    const dependencies = new Map<string, string[]>();
    const dependants = new Map<string, string[]>();
    const outputMatching = new Map<string, string>();
    for (const file of entryFiles) {
      dependencies.set(file, []);
      dependants.set(file, []);
      outputMatching.set(file, changeFileExtension(file, ABC_SUFFIX));
    }
    return { dependants, dependencies, outputMatching } as DependencyFileMap;
  }

  private createDependencyGraph(entryFiles: Set<string>, fileToModule: Map<string, ModuleInfo>, dependencyMap: DependencyFileMap): Graph<CompileJobInfo> {
    const dependencyGraphNodes: GraphNode<CompileJobInfo>[] = [];
    let jobType = CompileJobType.ABC;
    if (isHarOrHsp(this.mainModuleType) && ENABLE_DECL_FILE_CACHE) {
      jobType = CompileJobType.DECL_ABC;
    }

    for (const file of entryFiles) {
      const module = fileToModule.get(file);
      if (module === undefined) {
        throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
          'File does not belong to any module.', '', file));
      }
      const output = dependencyMap.outputMatching.get(file) ?? changeFileExtension(file, ABC_SUFFIX);
      const node = new GraphNode<CompileJobInfo>(computeHash(file), {
        contentType: JobContentType.FILE,
        content: {
          input: file,
          output: output,
        } as FileInfo,
        arktsConfig: module.arktsConfigFile,
        moduleName: module.packageName,
        moduleRoot: module.moduleRootPath,
        declgenConfig: {
          output: module.declgenV2OutPath ?? changeFileExtension(output, ETSCACHE_SUFFIX),
        },
        jobType: jobType,
      } as CompileJobInfo);

      const dependencies = dependencyMap.dependencies.get(file);
      if (dependencies !== undefined) {
        for (const dependency of dependencies) {
          node.predecessors.add(computeHash(dependency));
        }
      }
      const dependants = dependencyMap.dependants.get(file);
      if (dependants !== undefined) {
        for (const dependant of dependants) {
          node.descendants.add(computeHash(dependant));
        }
      }
      dependencyGraphNodes.push(node);
    }
    return Graph.createGraphFromNodes(dependencyGraphNodes);
  }

}

function changeFileExtension(filePath: string, newExtension: string): string {
  const idx = filePath.lastIndexOf('.');
  if (idx < 0) {
    return filePath + newExtension;
  }
  return filePath.substring(0, idx) + newExtension;
}
