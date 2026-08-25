/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import * as path from 'node:path';
import * as common from '@interop-toolkits/common';
import type { Context } from '../context';
import { INTEROP_ENTRY_FILES_ARTIFACT, type InteropEntryFiles } from './stageArtifacts';

export function createResolveInteropEntriesStage(): common.framework.pipeline.ProvidedStage<
  Context,
  readonly [],
  typeof INTEROP_ENTRY_FILES_ARTIFACT
> {
  return common.framework.pipeline.Stage.start<Context>('resolve-interop-entries')
    .use('resolve-interop-entries', {
      inputs: [],
      run: async (scope): Promise<InteropEntryFiles> => resolveInteropEntryFiles(scope.context),
    })
    .provides(INTEROP_ENTRY_FILES_ARTIFACT, {
      build: (_scope, outputs): InteropEntryFiles => outputs['resolve-interop-entries'],
    });
}

async function resolveInteropEntryFiles(context: Context): Promise<InteropEntryFiles> {
  const entryFiles: InteropEntryFiles = {
    staticEntryFiles: new Set<string>(),
    dynamicEntryFiles: new Set<string>(),
  };
  const moduleTable = createModuleTable(context.buildConfig);
  const interopConfig = await common.interopConfig.resolveInteropConfig(moduleTable);
  for (const [packageName, target] of interopConfig) {
    if (target.kind === 'items') {
      addFiles(entryFiles, target.staticFiles, target.dynamicFiles);
      continue;
    }
    const moduleInfo = context.fileManager.queryModuleInfo(packageName);
    if (moduleInfo === undefined) {
      throw new common.errors.InternalError(`Module info for package ${packageName} is not found.`);
    }
    addFiles(entryFiles, moduleInfo.staticFiles, moduleInfo.dynamicFiles);
  }
  return entryFiles;
}

function addFiles(entryFiles: InteropEntryFiles, staticFiles: Iterable<string>, dynamicFiles: Iterable<string>): void {
  for (const file of staticFiles) {
    entryFiles.staticEntryFiles.add(common.fileUtils.normalizePath(file));
  }
  for (const file of dynamicFiles) {
    entryFiles.dynamicEntryFiles.add(common.fileUtils.normalizePath(file));
  }
}

function createModuleTable(buildConfig: Context['buildConfig']): common.interopConfig.ModuleTable {
  const projectRootPath = common.fileUtils.normalizePath(buildConfig.projectRootPath);
  const modules = buildConfig.dependentModuleList.map((module) => {
    const modulePath = path.resolve(projectRootPath, module.modulePath);
    return {
      packageName: module.packageName,
      modulePath,
      dependencies: module.dependencies ?? [],
      ...(module.interopConfigPath === undefined || module.interopConfigPath === ''
        ? {}
        : { interopConfigPath: path.resolve(modulePath, module.interopConfigPath) }),
    };
  });
  const mainEntry = modules.find((module) => module.packageName === buildConfig.packageName);
  if (mainEntry === undefined) {
    throw new common.errors.InternalError(
      `Main module "${buildConfig.packageName}" is missing from dependentModuleList.`,
    );
  }
  const mainModule = {
    ...mainEntry,
    projectRootPath,
    cachePath: common.fileUtils.normalizePath(buildConfig.cachePath),
    outputRootPath: common.fileUtils.normalizePath(buildConfig.cachePath),
  };
  return {
    modules,
    byPackage: new Map(modules.map((module) => [module.packageName, module])),
    mainModule,
  };
}
