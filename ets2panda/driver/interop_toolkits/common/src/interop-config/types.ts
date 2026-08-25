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

export type InteropTarget =
  | {
      readonly kind: 'package';
      readonly moduleInfo: InteropConfigModuleInfo;
    }
  | {
      readonly kind: 'items';
      readonly staticFiles: readonly string[];
      readonly dynamicFiles: readonly string[];
    };

export interface InteropConfigModuleInfo {
  readonly packageName: string;
  readonly modulePath: string;
  readonly dependencies: readonly string[];
  readonly interopConfigPath?: string;
}

export interface MainModuleInfo extends InteropConfigModuleInfo {
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
  readonly modules: readonly InteropConfigModuleInfo[];
  readonly byPackage: ReadonlyMap<string, InteropConfigModuleInfo>;
  readonly mainModule: MainModuleInfo;
}
