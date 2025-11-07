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

import * as fs from 'fs';
import * as path from 'path';
import * as JSON5 from 'json5';
import { BuildConfig, PathConfig } from '../common/types';
import { DEFAULT_CACHE_DIR, EXTERNAL_API_PATH_FROM_SDK, LANGUAGE_VERSION } from '../common/preDefine';
import { getFileLanguageVersion } from '../common/utils';

export interface ModuleDescriptor {
  name: string;
  moduleType: string;
  srcPath: string;
  arktsversion?: string;
  aceModuleJsonPath?: string;
}

interface Json5Object {
  module?: {
    type?: string;
  };
  modules?: Array<{
    name: string;
    srcPath: string;
    arktsversion?: string;
  }>;
  dependencies?: {
    [packageName: string]: string;
  };
}

function removeTrailingCommas(jsonString: string): string {
  return jsonString.replace(/,\s*([}\]])/g, '$1');
}

function parseJson5(filePath: string): Json5Object {
  try {
    const rawContent = fs.readFileSync(filePath, 'utf8');
    const cleanedContent = removeTrailingCommas(rawContent);
    return JSON5.parse(cleanedContent) as Json5Object;
  } catch (error) {
    console.error(`Error parsing ${filePath}:`, error);
    return {} as Json5Object;
  }
}

function getModuleTypeFromConfig(modulePath: string): string {
  const moduleConfigPath = path.join(modulePath, 'src/main/module.json5');
  if (fs.existsSync(moduleConfigPath)) {
    try {
      const moduleData = parseJson5(moduleConfigPath);
      return moduleData.module?.type || 'har';
    } catch (error) {
      console.error(`Error parsing ${moduleConfigPath}:`, error);
    }
  }
  return 'har';
}

function getModulesFromBuildProfile(buildProfilePath: string): ModuleDescriptor[] {
  if (!fs.existsSync(buildProfilePath)) {
    console.error('Error: build-profile.json5 not found');
    process.exit(1);
  }

  const buildProfile = parseJson5(buildProfilePath);
  const modules = buildProfile.modules || [];

  return modules.map((module: { name: string; srcPath: string; arktsversion?: string }) => {
    const moduleSrcPath = path.resolve(path.dirname(buildProfilePath), module.srcPath);
    const arktsversion = module.arktsversion || '1.1';
    return {
      name: module.name,
      moduleType: getModuleTypeFromConfig(moduleSrcPath),
      srcPath: moduleSrcPath,
      arktsversion
    };
  });
}

function getEtsFiles(modulePath: string): string[] {
  const files: string[] = [];

  const shouldSkipDirectory = (relativePath: string): boolean => {
    const filterList = [`src${path.sep}test`, `src${path.sep}ohosTest`, `build${path.sep}`, `oh_modules${path.sep}`];
    return filterList.some((directoryPrefix: string) => relativePath.startsWith(directoryPrefix));
  };

  const processEntry = (dir: string, entry: fs.Dirent): void => {
    const fullPath = path.join(dir, entry.name);
    const relativePath = path.relative(modulePath, fullPath);

    if (entry.isDirectory()) {
      if (shouldSkipDirectory(relativePath)) {
        return;
      }
      traverseDir(fullPath);
      return;
    }

    if (entry.isFile() && entry.name.endsWith('.ets')) {
      files.push(fullPath);
    }
  };

  const traverseDir = (dir: string): void => {
    if (!fs.existsSync(dir)) {
      return;
    }

    const entries = fs.readdirSync(dir, { withFileTypes: true });
    entries.forEach((entry) => processEntry(dir, entry));
  };

  traverseDir(modulePath);
  return files;
}

function getModuleDependencies(modulePath: string, visited = new Set<string>()): string[] {
  if (visited.has(modulePath)) {
    return [];
  }
  visited.add(modulePath);

  const extractDependencies = (): string[] => {
    const packageFilePath = path.join(modulePath, 'oh-package.json5');
    if (!fs.existsSync(packageFilePath)) {
      return [];
    }

    try {
      const packageData = parseJson5(packageFilePath);
      return Object.entries(packageData.dependencies || {})
        .map(([_, depPath]) => (depPath as string).replace('file:', ''))
        .map((depPath) => path.resolve(modulePath, depPath));
    } catch (error) {
      console.error(`Error parsing ${packageFilePath}:`, error);
      return [];
    }
  };

  const dependencies = extractDependencies();
  return Array.from(new Set([...dependencies]));
}

function createMapEntryForPlugin(buildSdkPath: string, pluginName: string): string {
  return path.join(buildSdkPath, 'build-tools', 'ui-plugins', 'lib', pluginName, 'index');
}

function createPluginMap(buildSdkPath: string): Record<string, string> {
  let pluginMap: Record<string, string> = {};
  const pluginList: string[] = ['ui-syntax-plugins', 'ui-plugins', 'memo-plugins'];
  for (const plugin of pluginList) {
    pluginMap[plugin] = createMapEntryForPlugin(buildSdkPath, plugin);
  }
  return pluginMap;
}

function addPluginPathConfigs(buildConfig: BuildConfig, module: ModuleDescriptor): void {
  buildConfig.aceModuleJsonPath = module.aceModuleJsonPath;
}

function getModuleLanguageVersion(compileFiles: Set<string>): string {
  let found1_1 = false;
  let found1_2 = false;

  for (const file of compileFiles) {
    const sourceFile = fs.readFileSync(file, 'utf8');
    const languageVersion = getFileLanguageVersion(sourceFile);

    if (languageVersion === LANGUAGE_VERSION.ARKTS_1_2) {
      found1_2 = true;
    } else if (languageVersion === LANGUAGE_VERSION.ARKTS_1_1) {
      found1_1 = true;
    }

    if (found1_1 && found1_2) {
      return LANGUAGE_VERSION.ARKTS_HYBRID;
    }
  }

  return found1_2 ? LANGUAGE_VERSION.ARKTS_1_2 : found1_1 ? LANGUAGE_VERSION.ARKTS_1_1 : '';
}

export function generateBuildConfigs(
  pathConfig: PathConfig,
  modules?: ModuleDescriptor[]
): Record<string, BuildConfig> {
  const allBuildConfigs: Record<string, BuildConfig> = {};

  if (!modules) {
    const buildProfilePath = path.join(pathConfig.projectPath, 'build-profile.json5');
    modules = getModulesFromBuildProfile(buildProfilePath);
  }

  const definedModules = modules;

  const enableDeclgen: Map<string, boolean> = new Map(modules.map((module) => [module.name, false]));

  for (const module of definedModules) {
    const modulePath = module.srcPath;
    const compileFiles = new Set(getEtsFiles(modulePath));
    const pluginMap = createPluginMap(pathConfig.buildSdkPath);

    // Get recursive dependencies
    const dependencies = getModuleDependencies(modulePath);
    for (const depPath of dependencies) {
      getEtsFiles(depPath).forEach((file) => compileFiles.add(file));
      const depModule = definedModules.find((m) => m.srcPath === depPath);
      if (module.arktsversion === '1.1' && depModule?.arktsversion === '1.2') {
        enableDeclgen.set(depModule.name, true);
      }
    }
    let languageVersion = getModuleLanguageVersion(compileFiles);
    allBuildConfigs[module.name] = {
      plugins: pluginMap,
      compileFiles: Array.from(compileFiles),
      packageName: module.name,
      moduleType: module.moduleType,
      moduleRootPath: modulePath,
      language: languageVersion,
      buildSdkPath: pathConfig.buildSdkPath,
      projectPath: pathConfig.projectPath,
      declgenOutDir: pathConfig.declgenOutDir,
      externalApiPath: pathConfig.externalApiPath
        ? pathConfig.externalApiPath
        : path.resolve(pathConfig.buildSdkPath, EXTERNAL_API_PATH_FROM_SDK),
      cacheDir:
        pathConfig.cacheDir !== undefined ? pathConfig.cacheDir : path.join(pathConfig.projectPath, DEFAULT_CACHE_DIR),
      enableDeclgenEts2Ts: false,
      declFilesPath:
        languageVersion === LANGUAGE_VERSION.ARKTS_1_1
          ? path.join(pathConfig.declgenOutDir, 'static', module.name, 'decl-fileInfo.json')
          : undefined,
      dependencies: dependencies.map((dep) => {
        const depModule = definedModules.find((m) => m.srcPath === dep);
        return depModule!.name;
      })
    };
    addPluginPathConfigs(allBuildConfigs[module.name], module);
  }
  Object.entries(allBuildConfigs).forEach(([key, config]) => {
    if (enableDeclgen.get(key) === true) {
      config.enableDeclgenEts2Ts = true;
      config.declgenV1OutPath = path.join(pathConfig.declgenOutDir, 'dynamic', key, 'declgenV1');
      config.declgenBridgeCodePath = path.join(pathConfig.declgenOutDir, 'dynamic', key, 'declgenBridgeCode');
      if (!fs.existsSync(config.declgenV1OutPath)) {
        fs.mkdirSync(config.declgenV1OutPath, { recursive: true });
      }
      if (!fs.existsSync(config.declgenBridgeCodePath)) {
        fs.mkdirSync(config.declgenBridgeCodePath, { recursive: true });
      }
    }
  });
  return allBuildConfigs;
}
