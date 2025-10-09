/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import * as path from 'path';
import { BuildConfig, DependentModuleConfig } from '../types';
import {
  toUnixPath,
  readFirstLineSync
} from '../util/utils';
import { ETS_1_1, ETS_1_1_INTEROP, LANGUAGE_VERSION } from '../pre_define';

export class FileManager {
  private static instance: FileManager | undefined = undefined;
  static arkTSModuleMap: Map<string, DependentModuleConfig> = new Map();
  static staticApiPath: Set<string> = new Set();
  static dynamicApiPath: Set<string> = new Set();
  static buildConfig: BuildConfig;
  private constructor() { }
  static init(buildConfig: BuildConfig): void {
    if (FileManager.instance === undefined) {
      FileManager.instance = new FileManager();
      FileManager.initLanguageVersionFromDependentModuleMap(buildConfig.dependentModuleList);
      FileManager.initSDK(new Set(buildConfig.externalApiPaths), buildConfig.buildSdkPath);
      FileManager.buildConfig = buildConfig;
    }
  }

  static getInstance(): FileManager {
    if (!FileManager.instance) {
      FileManager.instance = new FileManager();
    }
    return FileManager.instance;
  }

  static cleanFileManagerObject(): void {
    if (this.instance) {
      this.instance = undefined;
    }
  }

  static initSDK(externalApiPath: Set<string>, buildSDKPath: string): void {
    externalApiPath?.forEach(path => {
      FileManager.staticApiPath.add(toUnixPath(path));
    });

    const etsPath = path.resolve(buildSDKPath, '../');

    FileManager.dynamicApiPath.add(toUnixPath(path.resolve(etsPath, ETS_1_1)));
    FileManager.dynamicApiPath.add(toUnixPath(path.resolve(etsPath, ETS_1_1_INTEROP)));
  }

  private static initLanguageVersionFromDependentModuleMap(
    dependentModuleList: DependentModuleConfig[]
  ): void {
    const convertedMap = new Map<string, DependentModuleConfig>();
    dependentModuleList.forEach(module => {
      const convertedModule: DependentModuleConfig = {
        ...module,
        modulePath: toUnixPath(module.modulePath),
        declgenV1OutPath: module.declgenV1OutPath ? toUnixPath(module.declgenV1OutPath) : undefined,
        declgenBridgeCodePath: module.declgenBridgeCodePath ? toUnixPath(module.declgenBridgeCodePath) : undefined,
        declFilesPath: module.declFilesPath ? toUnixPath(module.declFilesPath) : undefined,
      };
      convertedMap.set(module.packageName, convertedModule);
    });

    this.arkTSModuleMap = convertedMap;
  }
  private static isFirstLineUseStatic(filePath: string): boolean {
    const firstLine = readFirstLineSync(filePath);
    return firstLine === "'use static'";
  }

  getLanguageVersionByFilePath(filePath: string): string {
    const path = toUnixPath(filePath);
    for (const apiPath of FileManager.staticApiPath) {
      if (path.startsWith(apiPath)) {
        return LANGUAGE_VERSION.ARKTS_1_2;
      }
    }
    for (const apiPath of FileManager.dynamicApiPath) {
      if (path.startsWith(apiPath)) {
        return LANGUAGE_VERSION.ARKTS_1_1;
      }
    }
    if (FileManager.buildConfig.compileFiles?.includes(filePath)) {
      return LANGUAGE_VERSION.ARKTS_1_2;
    }
    for (const [pkgName, moduleInfo] of FileManager.arkTSModuleMap) {
      if (!path.startsWith(moduleInfo.modulePath)) {
        continue;
      }
      if (moduleInfo.language !== LANGUAGE_VERSION.ARKTS_HYBRID) {
        return moduleInfo.language;
      }
      /**
       * when process hybrid hsp or har we can't get info of 1.1,
       * only by module decl-fileinfo.json or `'use static'`
       */
      if (FileManager.isFirstLineUseStatic(filePath)) {
        return LANGUAGE_VERSION.ARKTS_1_2;
      }
    }
    return LANGUAGE_VERSION.ARKTS_1_1;
  }
}