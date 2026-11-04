/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
import * as fs from 'fs';

import { ARKTSCONFIG_JSON_FILE, LANGUAGE_VERSION } from '../common/preDefine';
import { BuildConfig, ModuleInfo } from '../common/types';
import { ArkTSConfigGenerator } from '../common/arkTSConfigGenerator';
import { logger } from './logger';

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

// Resolves the module entry file, mirroring the build system: an entry declared on the
// module descriptor wins (kept absolute), otherwise the legacy module-root 'Index.ets'
// and the hvigor 'src/main/ets/Index.ets' convention are probed (case-insensitively,
// since the decl file keys and the entry comparison are lowercased).
function resolveEntryFile(buildConfig: BuildConfig): string {
  if (buildConfig.entryFile) {
    return path.isAbsolute(buildConfig.entryFile)
      ? buildConfig.entryFile
      : path.resolve(buildConfig.moduleRootPath, buildConfig.entryFile);
  }
  if (buildConfig.packageName === 'entry') {
    return '';
  }
  const candidates = ['Index.ets', path.join('src', 'main', 'ets', 'Index.ets')];
  for (const candidate of candidates) {
    const entryPath = path.join(buildConfig.moduleRootPath, candidate);
    if (fs.existsSync(entryPath)) {
      return entryPath;
    }
    const dir = path.dirname(entryPath);
    if (fs.existsSync(dir) && fs.readdirSync(dir).some((name) => name.toLowerCase() === 'Index.ets')) {
      return entryPath;
    }
  }
  return path.join(buildConfig.moduleRootPath, 'Index.ets');
}

export function generateModuleInfo(allBuildConfig: Record<string, BuildConfig>, buildConfig: BuildConfig): ModuleInfo {
  let moduleInfo: ModuleInfo = {
    packageName: buildConfig.packageName,
    moduleRootPath: buildConfig.moduleRootPath,
    moduleType: buildConfig.moduleType,
    entryFile: resolveEntryFile(buildConfig),
    arktsConfigFile: path.resolve(buildConfig.cacheDir!, buildConfig.packageName, ARKTSCONFIG_JSON_FILE),
    compileFiles: buildConfig.compileFiles,
    depModuleCompileFiles: buildConfig.depModuleCompileFiles,
    declgenV1OutPath: buildConfig.declgenV1OutPath,
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

export function generateArkTsConfigs(allBuildConfig: Record<string, BuildConfig>, kitsFiles: string[] = []): Record<string, ModuleInfo> {
  let moduleInfos: Record<string, ModuleInfo> = collectModuleInfos(allBuildConfig);
  // The SDK kit files are module-independent: every module's generator traversal
  // returns the same set. Collect them once to avoid duplicates - compileKits
  // introduces a program per entry and a duplicated entry double-registers the
  // same resolved source (ImportPathManager::ResolvedSources::Register asserts).
  const collectedKits = new Set<string>();
  Object.keys(moduleInfos).forEach((packageName: string) => {
    let buildConfig = allBuildConfig[packageName];
    let generator = ArkTSConfigGenerator.getGenerator(buildConfig, moduleInfos);
    generator.writeArkTSConfigFile(moduleInfos[packageName]);
    generator.getkitsDirectoryFiles().forEach((kitFile: string) => collectedKits.add(kitFile));
  });
  kitsFiles.length = 0;
  kitsFiles.push(...collectedKits);
  let fileToModuleInfo: Record<string, ModuleInfo> = {};
  Object.values(moduleInfos).forEach((moduleInfo: ModuleInfo) => {
    moduleInfo.compileFiles.forEach((file: string) => {
      fileToModuleInfo[file] = moduleInfo;
    });
  });
  return fileToModuleInfo;
}
