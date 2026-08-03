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

import type { BuildConfig, ModuleConfig } from '../../contracts';
import { GlueGenError, GlueGenErrorCode, GlueGenInternalError, errorMessage } from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import type { MainModuleInfo, ModuleInfo, ModuleTable } from './moduleTable';
import {
  copyStringMap,
  copyStrings,
  normalizeOptionalPath,
  resolveDependencyNames,
  resolveFrom,
  stableUnique,
} from './moduleConfig';

/** Configuration hook: resolves the validated input into the shared module table. */
export function resolveModules(scope: StageScope<GlueGenContext, readonly []>): ModuleTable {
  try {
    return buildModuleTable(scope.context.buildConfig);
  } catch (error) {
    if (error instanceof GlueGenError) {
      throw error;
    }
    throw new GlueGenInternalError(
      new LogData({
        code: GlueGenErrorCode.INTERNAL_FAILURE,
        description: 'Gluegen could not resolve the module dependency table.',
        cause: errorMessage(error, 'unknown configuration failure'),
      }),
    );
  }
}

/** Normalizes validated module descriptors and indexes them by package name. */
function buildModuleTable(buildConfig: BuildConfig): ModuleTable {
  const projectRootPath = path.normalize(buildConfig.projectRootPath);
  const normalizedModules = buildConfig.dependentModuleList.map((module) => normalizeModule(module, projectRootPath));
  const mainEntry = normalizedModules.find((module) => module.packageName === buildConfig.packageName);
  if (mainEntry === undefined) {
    throw new GlueGenInternalError(
      new LogData({
        code: GlueGenErrorCode.INTERNAL_FAILURE,
        description: 'The validated Main Module is missing from the resolved module table.',
        cause: `Main package: ${buildConfig.packageName}`,
      }),
    );
  }

  const cacheRootPath = resolveFrom(projectRootPath, buildConfig.cachePath);
  const outputRootPath = resolveFrom(projectRootPath, 'gluegen-out');
  const mainModule: MainModuleInfo = {
    ...mainEntry,
    projectRootPath,
    cachePath: cacheRootPath,
    outputRootPath,
  };
  const modules: readonly ModuleInfo[] = normalizedModules;
  const byPackage = new Map<string, ModuleInfo>();
  for (const module of modules) {
    byPackage.set(module.packageName, module);
  }

  return {
    modules,
    byPackage,
    mainModule,
  };
}

function normalizeModule(module: ModuleConfig, projectRootPath: string): ModuleInfo {
  const modulePath = resolveFrom(projectRootPath, module.modulePath);
  const originalPackageNameMap = copyStringMap(
    module.originalPackageNameMap ?? {},
    `originalPackageNameMap for ${module.packageName}`,
  );
  const dependencies = resolveDependencyNames(
    copyStrings(module.dependencies ?? [], `dependencies for ${module.packageName}`),
    originalPackageNameMap,
  );

  const abcPath = normalizeOptionalPath(projectRootPath, module.abcPath);
  const declFilesPath = normalizeOptionalPath(projectRootPath, module.declFilesPath);
  const interopConfigPath = normalizeOptionalPath(modulePath, module.interopConfigPath);
  return {
    packageName: module.packageName,
    ...optionalProperty('moduleName', module.moduleName),
    ...optionalProperty('moduleType', module.moduleType),
    modulePath,
    sourceRoots: module.sourceRoots.map((root) => resolveFrom(modulePath, root)),
    entryFile: module.entryFile === '' ? '' : resolveFrom(modulePath, module.entryFile),
    dependencies: stableUnique(dependencies),
    ...optionalProperty('abcPath', abcPath),
    ...optionalProperty('declFilesPath', declFilesPath),
    ...optionalProperty('language', module.language),
    ...optionalProperty('bundleType', module.bundleType),
    ...optionalProperty('bundleName', module.bundleName),
    ...optionalProperty('packageVersion', module.packageVersion),
    originalPackageNameMap,
    ...optionalProperty('interopConfigPath', interopConfigPath),
  };
}

function optionalProperty<Key extends string, Value>(
  key: Key,
  value: Value | undefined,
): { readonly [Property in Key]?: Value } {
  if (value === undefined) {
    return {};
  }
  return { [key]: value } as { readonly [Property in Key]: Value };
}
