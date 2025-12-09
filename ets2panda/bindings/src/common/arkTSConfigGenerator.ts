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
import * as JSON5 from 'json5';

import { changeFileExtension, ensurePathExists, getFileLanguageVersion } from './utils';
import { AliasConfig, BuildConfig, ModuleInfo } from './types';
import { LANGUAGE_VERSION, PANDA_SDK_PATH_FROM_SDK, SYSTEM_SDK_PATH_FROM_SDK } from './preDefine';
import { logger } from '../lsp/logger';

interface DependencyItem {
  language: string;
  path: string;
  ohmUrl?: string;
  alias?: string[];
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
  private interopApiPath: string;

  private moduleInfos: Record<string, ModuleInfo>;
  private pathSection: Record<string, string[]>;

  private constructor(buildConfig: BuildConfig, moduleInfos: Record<string, ModuleInfo>) {
    let pandaSdkPath = path.resolve(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
    let pandaStdlibPath: string = path.resolve(pandaSdkPath, 'lib', 'stdlib');
    this.stdlibStdPath = path.resolve(pandaStdlibPath, 'std');
    this.stdlibEscompatPath = path.resolve(pandaStdlibPath, 'escompat');
    this.systemSdkPath = path.resolve(buildConfig.buildSdkPath, SYSTEM_SDK_PATH_FROM_SDK);
    this.externalApiPath = buildConfig.externalApiPath !== undefined ? buildConfig.externalApiPath : '';
    this.interopApiPath = buildConfig.interopApiPath !== undefined ? buildConfig.interopApiPath : '';

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

  private traverse(
    pathSection: Record<string, string[] | DependencyItem>,
    currentDir: string,
    aliasConfigObj: Record<string, AliasConfig> | undefined,
    prefix: string = '',
    isInteropSdk: boolean = false,
    relativePath: string = '',
    isExcludedDir: boolean = false,
    allowedExtensions: string[] = ['.d.ets']
  ): void {
    const items = fs.readdirSync(currentDir);
    for (const item of items) {
      const itemPath = path.join(currentDir, item);
      const stat = fs.statSync(itemPath);
      const isAllowedFile = allowedExtensions.some((ext) => item.endsWith(ext));
      const separator = isInteropSdk ? '/' : '.';
      if (stat.isFile() && !isAllowedFile) {
        continue;
      }

      if (stat.isFile()) {
        const basename = path.basename(item, '.d.ets');
        const key = isExcludedDir ? basename : relativePath ? `${relativePath}${separator}${basename}` : basename;
        pathSection[prefix + key] = isInteropSdk
          ? {
            language: 'js',
            path: itemPath,
            ohmUrl: '',
            alias: aliasConfigObj ? this.processAlias(basename, aliasConfigObj) : undefined
          }
          : [changeFileExtension(itemPath, '', '.d.ets')];
      }
      if (stat.isDirectory()) {
        // For files under api dir excluding arkui/runtime-api dir,
        // fill path section with `"pathFromApi.subdir.fileName" = [${absolute_path_to_file}]`;
        // For @koalaui files under arkui/runtime-api dir,
        // fill path section with `"fileName" = [${absolute_path_to_file}]`.
        const isCurrentDirExcluded = path.basename(currentDir) === 'arkui' && item === 'runtime-api';
        const newRelativePath = isCurrentDirExcluded ? '' : relativePath ? `${relativePath}${separator}${item}` : item;
        this.traverse(
          pathSection,
          path.resolve(currentDir, item),
          aliasConfigObj,
          prefix,
          isInteropSdk,
          newRelativePath,
          isCurrentDirExcluded || isExcludedDir
        );
      }
    }
  }

  private generateSystemSdkPathSection(pathSection: Record<string, string[]>): void {
    let directoryNames: string[] = ['api', 'arkts', 'kits'];
    directoryNames.forEach((dir) => {
      let systemSdkPath = path.resolve(this.systemSdkPath, dir);
      let externalApiPath = path.resolve(this.externalApiPath, dir);
      fs.existsSync(systemSdkPath)
        ? this.traverse(pathSection, systemSdkPath, undefined)
        : logger.debug(`sdk path ${systemSdkPath} not exist.`);
      fs.existsSync(externalApiPath)
        ? this.traverse(pathSection, externalApiPath, undefined)
        : logger.debug(`sdk path ${externalApiPath} not exist.`);
    });
  }

  private getAlias(fullPath: string, entryRoot: string, packageName: string): string {
    const normalizedFull = path.normalize(fullPath);
    const normalizedEntry = path.normalize(entryRoot);
    const entryDir = normalizedEntry.endsWith(path.sep) ? normalizedEntry : normalizedEntry + path.sep;
    if (!normalizedFull.startsWith(entryDir)) {
      throw new Error(`Path ${fullPath} is not under entry root ${entryRoot}`);
    }
    const relativePath = normalizedFull.substring(entryDir.length);
    const formatPath = path.join(packageName, relativePath).replace(/\\/g, '/');
    const alias = formatPath;
    return changeFileExtension(alias, '');
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
      if (depModuleInfo.language === LANGUAGE_VERSION.ARKTS_1_2) {
        this.pathSection[depModuleInfo.packageName] = [path.resolve(depModuleInfo.moduleRootPath)];
      } else if (depModuleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
        depModuleInfo.compileFiles.forEach((file) => {
          const firstLine = fs.readFileSync(file, 'utf-8').split('\n')[0];
          if (firstLine.includes('use static')) {
            this.pathSection[this.getAlias(file, depModuleInfo.moduleRootPath, depModuleInfo.packageName)] = [
              path.resolve(file)
            ];
          }
        });
      }
    });

    if (moduleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
      moduleInfo.compileFiles.forEach((file) => {
        const firstLine = fs.readFileSync(file, 'utf-8').split('\n')[0];
        if (getFileLanguageVersion(firstLine) === LANGUAGE_VERSION.ARKTS_1_2) {
          this.pathSection[this.getAlias(file, moduleInfo.moduleRootPath, moduleInfo.packageName)] = [
            path.resolve(file)
          ];
        }
      });
    }

    return this.pathSection;
  }

  private getOhmurl(file: string, moduleInfo: ModuleInfo): string {
    let unixFilePath: string = file.replace(/\\/g, '/');
    let ohmurl: string = moduleInfo.packageName + '/' + unixFilePath;
    return changeFileExtension(ohmurl, '');
  }

  private parseDeclFile(moduleInfo: ModuleInfo, dependencySection: Record<string, DependencyItem>): void {
    if (!moduleInfo.declFilesPath || !fs.existsSync(moduleInfo.declFilesPath)) {
      logger.error(`Module ${moduleInfo.packageName} depends on dynamic module ${moduleInfo.packageName}, but
          decl file not found on path ${moduleInfo.declFilesPath}`);
      return;
    }
    let declFilesObject = JSON.parse(fs.readFileSync(moduleInfo.declFilesPath, 'utf-8'));
    Object.keys(declFilesObject.files).forEach((file: string) => {
      let ohmurl: string = this.getOhmurl(file, moduleInfo);
      dependencySection[ohmurl] = {
        language: 'js',
        path: declFilesObject.files[file].declPath,
        ohmUrl: declFilesObject.files[file].ohmUrl
      };

      let absFilePath: string = path.resolve(moduleInfo.moduleRootPath, file);
      let entryFileWithoutExtension: string = changeFileExtension(moduleInfo.entryFile, '');
      if (absFilePath === entryFileWithoutExtension) {
        dependencySection[moduleInfo.packageName] = dependencySection[ohmurl];
      }
    });
  }

  private parseSdkAliasConfigFile(sdkAliasConfigFilePath?: string): Record<string, AliasConfig> | undefined {
    if (!sdkAliasConfigFilePath) {
      return;
    }
    const rawContent = fs.readFileSync(sdkAliasConfigFilePath, 'utf-8');
    const jsonData = JSON5.parse(rawContent);
    const aliasConfigObj: Record<string, AliasConfig> = {};
    for (const [aliasKey, config] of Object.entries(jsonData)) {
      const aliasConfig = config as AliasConfig;
      aliasConfigObj[aliasKey] = aliasConfig;
    }
    return aliasConfigObj;
  }

  private processAlias(basename: string, aliasConfigObj: Record<string, AliasConfig>): string[] | undefined {
    let alias: string[] = [];
    for (const [aliasName, aliasConfig] of Object.entries(aliasConfigObj)) {
      if (aliasConfig.isStatic) {
        continue;
      }
      if (basename === aliasConfig.originalAPIName) {
        alias.push(aliasName);
      }
    }
    if (alias.length !== 0) {
      return alias;
    }
  }

  private generateSystemSdkDependenciesSection(
    dependencySection: Record<string, DependencyItem>,
    moduleInfo: ModuleInfo
  ): void {
    const aliasConfigObj = this.parseSdkAliasConfigFile(moduleInfo.sdkAliasConfigPath);
    let directoryNames: string[] = ['api', 'arkts', 'kits', 'component'];
    directoryNames.forEach((dirName) => {
      const basePath = path.resolve(this.interopApiPath, dirName);
      if (!fs.existsSync(basePath)) {
        logger.debug(`interop sdk path ${basePath} not exist.`);
        return;
      }
      if (dirName === 'component') {
        this.traverse(dependencySection, basePath, aliasConfigObj, 'component/', true);
      } else {
        this.traverse(dependencySection, basePath, aliasConfigObj, 'dynamic/', true);
      }
    });
  }

  private getDependenciesSection(moduleInfo: ModuleInfo, dependencySection: Record<string, DependencyItem>): void {
    this.generateSystemSdkDependenciesSection(dependencySection, moduleInfo);
    let depModules: string[] = moduleInfo.dynamicDepModuleInfos;
    depModules.forEach((depModuleName: string) => {
      let depModuleInfo = this.moduleInfos[depModuleName];
      this.parseDeclFile(depModuleInfo, dependencySection);
    });

    if (moduleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
      this.parseDeclFile(moduleInfo, dependencySection);
    }
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
