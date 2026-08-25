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

import type { LoggerGetter } from '@interop-toolkits/common';

/** Function-injection seam, kept separate from the pure-data fields of `BuildConfig`. */
export interface InjectFunction {
  readonly getHvigorConsoleLogger?: LoggerGetter;
}

/** One module descriptor; the main module is the entry whose `packageName` matches the project's. */
export interface ModuleConfig {
  readonly packageName: string;
  readonly moduleName?: string;
  readonly moduleType?: string;
  readonly modulePath: string;
  readonly sourceRoots: readonly string[];
  readonly entryFile: string;
  readonly language?: string;
  readonly declFilesPath?: string;
  readonly dependencies?: readonly string[];
  readonly abcPath?: string;
  readonly byteCodeHar?: boolean;
  readonly bundleType?: string;
  readonly bundleName?: string;
  readonly packageVersion?: string;
  readonly declgenV1OutPath?: string;
  readonly declgenV2OutPath?: string;
  readonly originalPackageNameMap?: Readonly<Record<string, string>>;
  readonly interopConfigPath?: string;
  readonly staticFiles: readonly string[];
  readonly dynamicFiles: readonly string[];
  readonly isNative?: boolean;
}

export interface SdkPaths {
  readonly staticSdkPaths: readonly string[];
  readonly dynamicSdkPaths: readonly string[];
  readonly staticInteropSdkPaths: readonly string[];
  readonly dynamicInteropSdkPaths: readonly string[];
}

export interface AliasItem {
  originalAPIName: string;
  isStatic: boolean;
}

export type SdkAliasMap = Record<string, Record<string, AliasItem>>;

export interface BuildConfig extends InjectFunction {
  readonly plugins: Record<string, string>;
  readonly dynamicPlugins: Record<string, string>;
  readonly buildSdkPath: string;
  readonly buildDynamicSdkPath: string;
  readonly buildMode: 'Debug' | 'Release';
  readonly buildType: 'BUILD';
  readonly projectRootPath: string;
  readonly cachePath: string;
  readonly compileSdkVersion: number;
  readonly compatibleSdkVersion: number;
  readonly bundleName: string;
  readonly moduleType?: string;
  readonly moduleName: string;
  readonly packageName: string;
  readonly dependentModuleList: readonly ModuleConfig[];
  readonly hasMainModule: boolean;
  readonly modulePath: string;
  readonly byteCodeHar: boolean;
  readonly declgenBridgeConfigPath: string;
  readonly interopConfigPath: string;
  readonly externalApiPaths: readonly string[];
  readonly sdkPaths: SdkPaths;
  readonly pandaSdkPath?: string;
  readonly sdkAliasMap: SdkAliasMap;
}
