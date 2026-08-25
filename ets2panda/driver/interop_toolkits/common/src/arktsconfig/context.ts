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

import * as path from 'node:path';

export interface DynamicInteropFile {
  readonly declPath: string;
  readonly filePath: string;
  readonly ohmUrl: string;
}

/** The in-memory equivalent of the dynamic interop context JSON. */
export interface DynamicInteropContext {
  readonly packageName: string;
  readonly files: Readonly<Record<string, DynamicInteropFile>>;
}

export interface ModuleInput {
  readonly packageName: string;
  readonly moduleName?: string;
  readonly moduleType?: string;
  readonly modulePath: string;
  readonly sourceRoots: readonly string[];
  readonly entryFile: string;
  readonly language?: string;
  readonly dependencies?: readonly string[];
  readonly abcPath?: string;
  readonly bundleType?: string;
  readonly bundleName?: string;
  readonly packageVersion?: string;
  readonly originalPackageNameMap?: Readonly<Record<string, string>>;
}

export interface ModuleInfo {
  readonly packageName: string;
  readonly moduleName?: string;
  readonly moduleType?: string;
  readonly modulePath: string;
  readonly sourceRoots: readonly string[];
  readonly entryFile: string;
  readonly language?: string;
  readonly dependencies: readonly string[];
  readonly abcPath?: string;
  readonly bundleType?: string;
  readonly bundleName?: string;
  readonly packageVersion?: string;
  readonly originalPackageNameMap: Readonly<Record<string, string>>;
}

export interface ArkTSConfigSourceContext {
  readonly projectRootPath: string;
  readonly cachePath: string;
  readonly pandaSdkPath: string;
  readonly packageName: string;
  readonly bundleName: string;
  readonly moduleType?: string;
  readonly modules: readonly ModuleInfo[];
  readonly interopContexts: ReadonlyMap<string, DynamicInteropContext>;
  readonly externalApiPaths: readonly string[];
  readonly dynamicInteropSdkPaths: readonly string[];
}

export interface ArkTSConfigSourceInput {
  readonly projectRootPath: string;
  readonly cachePath: string;
  readonly pandaSdkPath: string;
  readonly packageName: string;
  readonly bundleName: string;
  readonly moduleType?: string;
  readonly dependentModuleList: readonly ModuleInput[];
  readonly interopContexts: ReadonlyMap<string, DynamicInteropContext>;
  readonly externalApiPaths: readonly string[];
  readonly dynamicInteropSdkPaths: readonly string[];
}

export interface ArkTSConfigContext {
  readonly projectRootPath: string;
  readonly mainModule: ModuleInfo;
  readonly byPackage: ReadonlyMap<string, ModuleInfo>;
  readonly modules: readonly ModuleInfo[];
  readonly interopContexts: ReadonlyMap<string, DynamicInteropContext>;
  readonly cachePath: string;
  readonly pandaSdkPath: string;
  readonly bundleName: string;
  readonly moduleType?: string;
  readonly externalApiPaths: readonly string[];
  readonly dynamicInteropSdkPaths: readonly string[];
}

export function createArkTSConfigSourceContext(input: ArkTSConfigSourceInput): ArkTSConfigSourceContext {
  const projectRootPath = normalizePath(input.projectRootPath);
  return {
    projectRootPath,
    cachePath: resolveFrom(projectRootPath, input.cachePath),
    pandaSdkPath: resolveFrom(projectRootPath, input.pandaSdkPath),
    packageName: input.packageName,
    bundleName: input.bundleName,
    ...(input.moduleType === undefined ? {} : { moduleType: input.moduleType }),
    modules: input.dependentModuleList.map((module) => normalizeModule(module, projectRootPath)),
    interopContexts: new Map(input.interopContexts),
    externalApiPaths: input.externalApiPaths.map((value) => resolveFrom(projectRootPath, value)),
    dynamicInteropSdkPaths: input.dynamicInteropSdkPaths.map((value) => resolveFrom(projectRootPath, value)),
  };
}

export function createArkTSConfigContext(source: ArkTSConfigSourceContext): ArkTSConfigContext {
  const modulesByPackage = new Map(source.modules.map((module) => [module.packageName, module]));
  const mainModule = modulesByPackage.get(source.packageName);
  if (mainModule === undefined) {
    throw new Error(`Main module "${source.packageName}" was not found.`);
  }
  return {
    projectRootPath: source.projectRootPath,
    cachePath: source.cachePath,
    pandaSdkPath: source.pandaSdkPath,
    bundleName: source.bundleName,
    ...(source.moduleType === undefined ? {} : { moduleType: source.moduleType }),
    mainModule,
    modules: reachableModulesOf(mainModule, modulesByPackage),
    byPackage: modulesByPackage,
    interopContexts: source.interopContexts,
    externalApiPaths: source.externalApiPaths,
    dynamicInteropSdkPaths: source.dynamicInteropSdkPaths,
  };
}

function normalizeModule(module: ModuleInput, projectRootPath: string): ModuleInfo {
  const modulePath = resolveFrom(projectRootPath, module.modulePath);
  const originalPackageNameMap = { ...(module.originalPackageNameMap ?? {}) };
  const dependencies = stableUnique(
    (module.dependencies ?? []).map((dependency) => originalPackageNameMap[dependency] ?? dependency),
  );
  const abcPath = normalizeOptionalPath(projectRootPath, module.abcPath);

  return {
    packageName: module.packageName,
    ...(module.moduleName === undefined ? {} : { moduleName: module.moduleName }),
    ...(module.moduleType === undefined ? {} : { moduleType: module.moduleType }),
    modulePath,
    sourceRoots: module.sourceRoots.map((root) => resolveFrom(modulePath, root)),
    entryFile: module.entryFile === '' ? '' : resolveFrom(modulePath, module.entryFile),
    ...(module.language === undefined ? {} : { language: module.language }),
    dependencies,
    ...(abcPath === undefined ? {} : { abcPath }),
    ...(module.bundleType === undefined ? {} : { bundleType: module.bundleType }),
    ...(module.bundleName === undefined ? {} : { bundleName: module.bundleName }),
    ...(module.packageVersion === undefined ? {} : { packageVersion: module.packageVersion }),
    originalPackageNameMap,
  };
}

function reachableModulesOf(
  root: ModuleInfo,
  modulesByPackage: ReadonlyMap<string, ModuleInfo>,
): readonly ModuleInfo[] {
  const modules: ModuleInfo[] = [];
  const visited = new Set<string>();
  const visit = (module: ModuleInfo): void => {
    if (visited.has(module.packageName)) {
      return;
    }
    visited.add(module.packageName);
    modules.push(module);
    for (const dependencyName of module.dependencies) {
      const dependency = modulesByPackage.get(dependencyName);
      if (dependency !== undefined) {
        visit(dependency);
      }
    }
  };
  visit(root);
  return modules;
}

function normalizePath(fileName: string): string {
  return path.resolve(fileName).replace(/\\/g, '/');
}

function resolveFrom(basePath: string, candidate: string): string {
  return normalizePath(path.isAbsolute(candidate) ? candidate : path.resolve(basePath, candidate));
}

function normalizeOptionalPath(basePath: string, candidate: string | undefined): string | undefined {
  return candidate === undefined || candidate === '' ? undefined : resolveFrom(basePath, candidate);
}

function stableUnique(values: readonly string[]): readonly string[] {
  return [...new Set(values)];
}
