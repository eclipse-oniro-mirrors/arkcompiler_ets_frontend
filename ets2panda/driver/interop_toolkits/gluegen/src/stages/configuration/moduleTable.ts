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

export interface ModuleInfo {
  readonly packageName: string;
  readonly moduleName?: string;
  readonly moduleType?: string;
  readonly modulePath: string;
  readonly sourceRoots: readonly string[];
  readonly entryFile: string;
  readonly dependencies: readonly string[];
  readonly abcPath?: string;
  readonly declFilesPath?: string;
  readonly language?: string;
  readonly bundleType?: string;
  readonly bundleName?: string;
  readonly packageVersion?: string;
  readonly originalPackageNameMap: Readonly<Record<string, string>>;
  readonly interopConfigPath?: string;
}

export interface MainModuleInfo extends ModuleInfo {
  readonly projectRootPath: string;
  readonly cachePath: string;
  readonly outputRootPath: string;
}

/**
 * Immutable, pre-indexed module dependency table.
 *
 * Carries the normalized modules plus an O(1) `byPackage` lookup and the resolved main
 * module. Dependency *edges* are materialized on demand via `dependencyModulesOf`, so
 * consumers traverse real `ModuleInfo` references without rebuilding an index. Cycles are
 * rejected by the configuration hooks before this artifact is published.
 */
export interface ModuleTable {
  readonly modules: readonly ModuleInfo[];
  readonly byPackage: ReadonlyMap<string, ModuleInfo>;
  readonly mainModule: MainModuleInfo;
}

/** Resolves a module's dependency names to their immutable table entries. */
export function dependencyModulesOf(table: ModuleTable, module: ModuleInfo): readonly ModuleInfo[] {
  return module.dependencies
    .map((packageName) => table.byPackage.get(packageName))
    .filter((dependency): dependency is ModuleInfo => dependency !== undefined);
}

/** Finds a module and all its direct and transitive dependencies once, in traversal order. */
export function reachableModulesOf(table: ModuleTable, root: ModuleInfo = table.mainModule): readonly ModuleInfo[] {
  const modules: ModuleInfo[] = [];
  const visited = new Set<string>();
  const visit = (module: ModuleInfo): void => {
    if (visited.has(module.packageName)) {
      return;
    }
    visited.add(module.packageName);
    modules.push(module);
    dependencyModulesOf(table, module).forEach(visit);
  };
  visit(root);
  return modules;
}
