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
import * as fs from 'fs';

import { changeFileExtension, ensurePathExists } from './utils';
import { BuildConfig, ModuleInfo } from './types';
import { LANGUAGE_VERSION, PANDA_SDK_PATH_FROM_SDK, SYSTEM_SDK_PATH_FROM_SDK } from './preDefine';

interface DependencyItem {
  language: string;
  path: string;
  ohmUrl: string;
}

interface ArkTSConfigObject {
  compilerOptions: {
    package: string;
    baseUrl: string;
    paths: Record<string, string[]>;
    entry: string;
    dependencies: Record<string, DependencyItem>;
  };
}

export class ArkTSConfigGenerator {
  private static instance: ArkTSConfigGenerator | undefined;
  private stdlibStdPath: string;
  private stdlibEscompatPath: string;
  private systemSdkPath: string;
  private externalApiPath: string;

  private moduleInfos: Record<string, ModuleInfo>;
  private pathSection: Record<string, string[]>;

  private constructor(buildConfig: BuildConfig, moduleInfos: Record<string, ModuleInfo>) {
    let pandaSdkPath = path.resolve(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
    let pandaStdlibPath: string = path.resolve(pandaSdkPath, 'lib', 'stdlib');
    this.stdlibStdPath = path.resolve(pandaStdlibPath, 'std');
    this.stdlibEscompatPath = path.resolve(pandaStdlibPath, 'escompat');
    this.systemSdkPath = path.resolve(buildConfig.buildSdkPath, SYSTEM_SDK_PATH_FROM_SDK);
    this.externalApiPath = buildConfig.externalApiPath !== undefined ? buildConfig.externalApiPath : '';

    this.moduleInfos = moduleInfos;
    this.pathSection = {};
  }

  public static getInstance(buildConfig?: BuildConfig, moduleInfos?: Record<string, ModuleInfo>): ArkTSConfigGenerator {
    if (!ArkTSConfigGenerator.instance) {
      if (!buildConfig || !moduleInfos) {
        throw new Error('buildConfig and moduleInfos is required for the first instantiation of ArkTSConfigGenerator.');
      }
      ArkTSConfigGenerator.instance = new ArkTSConfigGenerator(buildConfig, moduleInfos);
    }
    return ArkTSConfigGenerator.instance;
  }

  public static getGenerator(buildConfig: BuildConfig, moduleInfos: Record<string, ModuleInfo>): ArkTSConfigGenerator {
    return new ArkTSConfigGenerator(buildConfig, moduleInfos);
  }

  public static destroyInstance(): void {
    ArkTSConfigGenerator.instance = undefined;
  }

  private generateSystemSdkPathSection(pathSection: Record<string, string[]>): void {
    function traverse(
      currentDir: string,
      relativePath: string = '',
      isExcludedDir: boolean = false,
      allowedExtensions: string[] = ['.d.ets']
    ): void {
      const items = fs.readdirSync(currentDir);
      for (const item of items) {
        const itemPath = path.join(currentDir, item);
        const stat = fs.statSync(itemPath);
        const isAllowedFile = allowedExtensions.some((ext) => item.endsWith(ext));
        if (stat.isFile() && !isAllowedFile) {
          continue;
        }

        if (stat.isFile()) {
          const basename = path.basename(item, '.d.ets');
          const key = isExcludedDir ? basename : relativePath ? `${relativePath}.${basename}` : basename;
          pathSection[key] = [changeFileExtension(itemPath, '', '.d.ets')];
        }
        if (stat.isDirectory()) {
          // For files under api dir excluding arkui/runtime-api dir,
          // fill path section with `"pathFromApi.subdir.fileName" = [${absolute_path_to_file}]`;
          // For @koalaui files under arkui/runtime-api dir,
          // fill path section with `"fileName" = [${absolute_path_to_file}]`.
          const isCurrentDirExcluded = path.basename(currentDir) === 'arkui' && item === 'runtime-api';
          const newRelativePath = isCurrentDirExcluded ? '' : relativePath ? `${relativePath}.${item}` : item;
          traverse(path.resolve(currentDir, item), newRelativePath, isCurrentDirExcluded || isExcludedDir);
        }
      }
    }

    let directoryNames: string[] = ['api', 'arkts', 'kits'];
    directoryNames.forEach((dir) => {
      let systemSdkPath = path.resolve(this.systemSdkPath, dir);
      let externalApiPath = path.resolve(this.externalApiPath, dir);
      fs.existsSync(systemSdkPath) ? traverse(systemSdkPath) : console.warn(`sdk path ${systemSdkPath} not exist.`);
      fs.existsSync(externalApiPath)
        ? traverse(externalApiPath)
        : console.warn(`sdk path ${externalApiPath} not exist.`);
    });
  }

  private getPathSection(moduleInfo: ModuleInfo): Record<string, string[]> {
    if (Object.keys(this.pathSection).length !== 0) {
      return this.pathSection;
    }

    this.pathSection.std = [this.stdlibStdPath];
    this.pathSection.escompat = [this.stdlibEscompatPath];

    this.generateSystemSdkPathSection(this.pathSection);

    Object.values(moduleInfo.staticDepModuleInfos).forEach((depModuleName: string) => {
      let depModuleInfo = this.moduleInfos[depModuleName];
      this.pathSection[depModuleInfo.packageName] = [path.resolve(depModuleInfo.moduleRootPath)];
    });

    return this.pathSection;
  }

  private getOhmurl(file: string, moduleInfo: ModuleInfo): string {
    let unixFilePath: string = file.replace(/\\/g, '/');
    let ohmurl: string = moduleInfo.packageName + '/' + unixFilePath;
    return changeFileExtension(ohmurl, '');
  }

  private getDependenciesSection(moduleInfo: ModuleInfo, dependencySection: Record<string, DependencyItem>): void {
    let depModules: string[] = moduleInfo.dynamicDepModuleInfos;
    depModules.forEach((depModuleName: string) => {
      let depModuleInfo = this.moduleInfos[depModuleName];
      if (!depModuleInfo.declFilesPath || !fs.existsSync(depModuleInfo.declFilesPath)) {
        console.error(`Module ${moduleInfo.packageName} depends on dynamic module ${depModuleInfo.packageName}, but
          decl file not found on path ${depModuleInfo.declFilesPath}`);
        return;
      }
      let declFilesObject = JSON.parse(fs.readFileSync(depModuleInfo.declFilesPath, 'utf-8'));
      Object.keys(declFilesObject.files).forEach((file: string) => {
        let ohmurl: string = this.getOhmurl(file, depModuleInfo);
        dependencySection[ohmurl] = {
          language: 'js',
          path: declFilesObject.files[file].declPath,
          ohmUrl: declFilesObject.files[file].ohmUrl
        };

        let absFilePath: string = path.resolve(depModuleInfo.moduleRootPath, file);
        let entryFileWithoutExtension: string = changeFileExtension(depModuleInfo.entryFile, '');
        if (absFilePath === entryFileWithoutExtension) {
          dependencySection[depModuleInfo.packageName] = dependencySection[ohmurl];
        }
      });
    });
  }

  public writeArkTSConfigFile(moduleInfo: ModuleInfo): void {
    let pathSection = this.getPathSection(moduleInfo);
    let dependencySection: Record<string, DependencyItem> = {};
    this.getDependenciesSection(moduleInfo, dependencySection);

    let baseUrl: string = path.resolve(moduleInfo.moduleRootPath);
    let arktsConfig: ArkTSConfigObject = {
      compilerOptions: {
        package: moduleInfo.packageName,
        baseUrl: baseUrl,
        paths: pathSection,
        entry: moduleInfo.entryFile,
        dependencies: dependencySection
      }
    };

    ensurePathExists(moduleInfo.arktsConfigFile);
    fs.writeFileSync(moduleInfo.arktsConfigFile, JSON.stringify(arktsConfig, null, 2), 'utf-8');
  }
}
