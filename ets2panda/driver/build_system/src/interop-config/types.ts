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

/** A single interop file declared in interop-config.json5, resolved against its owning module. */
export interface InteropFileEntry {
  /** Package name of the module the file belongs to. */
  readonly packageName: string;
  /** Path exactly as written in the interop configuration. */
  readonly relativePath: string;
  /** Absolute path resolved against the owning module's root. */
  readonly filePath: string;
}

export type InteropTarget =
  | {
      readonly kind: 'package';
      readonly moduleInfo: InteropConfigModuleInfo;
    }
  | {
      readonly kind: 'items';
      readonly staticFiles: readonly InteropFileEntry[];
      readonly dynamicFiles: readonly InteropFileEntry[];
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

/** Build options parsed from the project-level interop-config.json5. */
export interface InteropBuildOption {
  readonly stripInteropMapping: boolean;
}

/** Only the projection-level fields consumed by the build system; other interop fields are ignored. */
export interface ProjectInteropConfigFile {
  readonly interopBuildOption?: {
    readonly stripInteropMapping?: boolean;
  };
}

/** The projection interop config with defaults applied to every consumed field. */
export interface ResolvedProjectInteropConfigFile {
  readonly interopBuildOption: InteropBuildOption;
}

/**
 * Aggregated interop-config resolution result.
 *
 * Combines the project-level build options with the module-level interop targets.
 * An empty `targets` map means no module declares interop entries.
 */
export interface ResolvedInteropConfig {
  readonly buildOption: InteropBuildOption;
  readonly targets: ReadonlyMap<string, InteropTarget>;
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
