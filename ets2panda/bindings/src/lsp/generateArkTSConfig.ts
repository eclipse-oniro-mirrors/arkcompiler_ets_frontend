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

import { ARKTSCONFIG_JSON_FILE, LANGUAGE_VERSION } from '../common/preDefine';
import { BuildConfig, ModuleInfo } from '../common/types';
import { ArkTSConfigGenerator } from '../common/arkTSConfigGenerator';

function collectDepModuleInfos(moduleInfo: ModuleInfo, allBuildConfig: Record<string, BuildConfig>): void {
  let dynamicDepModules: string[] = [];
  let staticDepModules: string[] = [];

  if (moduleInfo.dependencies) {
    moduleInfo.dependencies.forEach((moduleName: string) => {
      let depModule = allBuildConfig[moduleName];
      if (depModule.language === LANGUAGE_VERSION.ARKTS_1_2) {
        staticDepModules.push(depModule.packageName);
      } else if (depModule.language === LANGUAGE_VERSION.ARKTS_1_1) {
        dynamicDepModules.push(depModule.packageName);
      } else {
        staticDepModules.push(depModule.packageName);
        dynamicDepModules.push(depModule.packageName);
      }
    });
  }
  moduleInfo.dynamicDepModuleInfos = dynamicDepModules;
  moduleInfo.staticDepModuleInfos = staticDepModules;
}

function collectModuleInfos(allBuildConfig: Record<string, BuildConfig>): Record<string, ModuleInfo> {
  let moduleInfos: Record<string, ModuleInfo> = {};
  Object.values(allBuildConfig).forEach((buildConfig) => {
    let moduleInfo = generateModuleInfo(allBuildConfig, buildConfig);
    moduleInfos[moduleInfo.packageName] = moduleInfo;
  });
  return moduleInfos;
}

export function generateModuleInfo(allBuildConfig: Record<string, BuildConfig>, buildConfig: BuildConfig): ModuleInfo {
  if (!buildConfig.packageName || !buildConfig.moduleRootPath) {
    console.error('Main buildConfig info from hvigor is not correct.');
  }
  let moduleInfo: ModuleInfo = {
    packageName: buildConfig.packageName,
    moduleRootPath: buildConfig.moduleRootPath,
    moduleType: buildConfig.moduleType,
    entryFile: buildConfig.packageName !== 'entry' ? path.join(buildConfig.moduleRootPath, 'Index.ets') : '',
    arktsConfigFile: path.resolve(buildConfig.cacheDir!, buildConfig.packageName, ARKTSCONFIG_JSON_FILE),
    compileFiles: buildConfig.compileFiles,
    depModuleCompileFiles: buildConfig.depModuleCompileFiles,
    declgenV1OutPath: buildConfig.declgenV1OutPath,
    declgenBridgeCodePath: buildConfig.declgenBridgeCodePath,
    staticDepModuleInfos: [],
    dynamicDepModuleInfos: [],
    language: buildConfig.language,
    dependencies: buildConfig.dependencies,
    declFilesPath: buildConfig.declFilesPath,
    sdkAliasConfigPath: buildConfig.sdkAliasConfigPath ? buildConfig.sdkAliasConfigPath : undefined
  };
  collectDepModuleInfos(moduleInfo, allBuildConfig);
  return moduleInfo;
}

export function generateArkTsConfigs(allBuildConfig: Record<string, BuildConfig>): Record<string, ModuleInfo> {
  let moduleInfos: Record<string, ModuleInfo> = collectModuleInfos(allBuildConfig);
  Object.keys(moduleInfos).forEach((packageName: string) => {
    let buildConfig = allBuildConfig[packageName];
    let generator = ArkTSConfigGenerator.getGenerator(buildConfig, moduleInfos);
    generator.writeArkTSConfigFile(moduleInfos[packageName]);
  });
  let fileToModuleInfo: Record<string, ModuleInfo> = {};
  Object.values(moduleInfos).forEach((moduleInfo: ModuleInfo) => {
    moduleInfo.compileFiles.forEach((file: string) => {
      fileToModuleInfo[file] = moduleInfo;
    });
  });
  return fileToModuleInfo;
}
