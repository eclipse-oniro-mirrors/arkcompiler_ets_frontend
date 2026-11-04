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
import * as JSON5 from 'json5';

import { changeFileExtension, ensurePathExists, getFileLanguageVersion } from './utils';
import { AliasConfig, BuildConfig, ModuleInfo } from './types';
import { EXTERNAL_API_PATH_FROM_SDK, LANGUAGE_VERSION, PANDA_SDK_PATH_FROM_SDK, SYSTEM_SDK_PATH_FROM_SDK } from './preDefine';
import { logger } from '../lsp/logger';

// The source root of an ArkTS module, by hvigor convention (see driver/build_system
// generate_arktsconfig.ts handleAddSourceRootPath: 'hvigor now use src/main as sourceRoot').
const MODULE_SOURCE_ROOT = 'src/main';

interface DependencyItem {
  language: string;
  path: string;
  sourceFilePath?: string;
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
  private stdlibArkruntimePath: string;
  private systemSdkPath: string;
  private externalApiPath: string;
  private interopApiPath: string;

  private moduleInfos: Record<string, ModuleInfo>;
  private pathSection: Record<string, string[]>;
  private kitsDirectoryFiles: string[];

  private constructor(buildConfig: BuildConfig, moduleInfos: Record<string, ModuleInfo>) {
    let pandaSdkPath = path.resolve(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
    let pandaStdlibPath: string = path.resolve(pandaSdkPath, 'lib', 'stdlib');
    this.stdlibStdPath = path.resolve(pandaStdlibPath, 'std');
    this.stdlibEscompatPath = path.resolve(pandaStdlibPath, 'escompat');
    this.stdlibArkruntimePath = path.resolve(pandaStdlibPath, 'arkruntime');
    this.systemSdkPath = path.resolve(buildConfig.buildSdkPath, SYSTEM_SDK_PATH_FROM_SDK);
    this.externalApiPath = buildConfig.externalApiPath !== undefined ? buildConfig.externalApiPath : '';
    this.interopApiPath = buildConfig.interopApiPath !== undefined ? buildConfig.interopApiPath : '';

    this.moduleInfos = moduleInfos;
    this.pathSection = {};
    this.kitsDirectoryFiles = [];
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

  public getkitsDirectoryFiles(): string[] {
    return this.kitsDirectoryFiles;
  }

  public getsystemSdkPath(): string {
    return this.systemSdkPath;
  }

  private traverse(
    pathSection: Record<string, string[] | DependencyItem>,
    currentDir: string,
    aliasConfigObj: Record<string, AliasConfig> | undefined,
    prefix: string = '',
    isInteropSdk: boolean = false,
    relativePath: string = '',
    isExcludedDir: boolean = false,
    allowedExtensions: string[] = ['.d.ets'],
    collectKitsFile: boolean = false,
    onlyApiFiles: boolean = false
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
      // The SDK api directory mixes legacy category subdirectories (ability/,
      // application/, ...) with the importable '@' prefixed declarations; only the
      // latter are addressable through the module syntax ('@ohos.xxx' /
      // 'dynamic@ohos.xxx'), the rest must not be collected.
      if (onlyApiFiles && !item.startsWith('@')) {
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
            alias: this.buildInteropSdkAlias(key, basename, aliasConfigObj)
          }
          : [changeFileExtension(itemPath, '', '.d.ets')];
        if (collectKitsFile && isAllowedFile) {
          this.kitsDirectoryFiles.push(itemPath);
        }
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
          isCurrentDirExcluded || isExcludedDir,
          allowedExtensions,
          collectKitsFile,
          onlyApiFiles
        );
      }
    }
  }

  // Mirrors the build system (driver/build_system generate_arktsconfig.ts processDynamicFile):
  // every interop SDK dependency carries itself as an alias ('<name>' and 'dynamic<name>'),
  // so the dynamic-side declarations' own bare imports (e.g. "@ohos.lang.interop") resolve
  // through the arktsconfig dependency-alias lookup instead of failing with
  // IMPORT_CANT_FIND_PREFIX.
  private buildInteropSdkAlias(
    key: string,
    basename: string,
    aliasConfigObj: Record<string, AliasConfig> | undefined
  ): string[] {
    const alias = [key, `dynamic${key}`];
    const sdkAliases = aliasConfigObj ? this.processAlias(basename, aliasConfigObj) : undefined;
    return Array.from(new Set(sdkAliases ? [...alias, ...sdkAliases] : alias));
  }

  private generateSystemSdkPathSection(pathSection: Record<string, string[]>): void {
    const directoryNames: string[] = ['api', 'arkts', 'kits'];
    const collectPath = (sdkPath: string, dir: string): void => {
      if (!fs.existsSync(sdkPath)) {
        logger.debug(`sdk path ${sdkPath} not exist.`);
        return;
      }
      // The SDK api directory mixes importable '@' prefixed declarations with legacy
      // category subdirectories (arkui/, application/, ...). The category files are
      // imported by the '@' declarations themselves (e.g. 'arkui.SymbolGlyphModifier'),
      // so they must stay in the path section; no '@' filtering is applied here.
      if (dir === 'kits') {
        this.traverse(pathSection, sdkPath, undefined, '', false, '', false, ['.d.ets'], true);
        return;
      }
      this.traverse(pathSection, sdkPath, undefined);
    };

    directoryNames.forEach((dir) => {
      const systemSdkPath = path.resolve(this.systemSdkPath, dir);
      const externalApiPath = path.resolve(this.externalApiPath, dir);
      collectPath(systemSdkPath, dir);
      collectPath(externalApiPath, dir);
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
    this.pathSection.arkruntime = [this.stdlibArkruntimePath];

    this.generateSystemSdkPathSection(this.pathSection);

    Object.values(moduleInfo.staticDepModuleInfos).forEach((depModuleName: string) => {
      let depModuleInfo = this.moduleInfos[depModuleName];
      if (depModuleInfo.language === LANGUAGE_VERSION.ARKTS_1_2) {
        this.pathSection[depModuleInfo.packageName] = [path.resolve(depModuleInfo.moduleRootPath)];
      } else if (depModuleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
        // Mirrors the build system: a hybrid module also carries the module-root mapping
        // ('addPathSection' registers every non-1.1 module root and the consumer merges
        // the dependency config in), in addition to its per-file static aliases.
        this.pathSection[depModuleInfo.packageName] = [path.resolve(depModuleInfo.moduleRootPath)];
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

  // Mirrors the build system (driver/build_system generate_arktsconfig.ts): dependency keys
  // keep the module relative file path ('dynlib/src/main/ets/Index') and are additionally
  // registered with the source root prefix stripped ('src/main' by hvigor convention), e.g.
  // 'dynlib/ets/Index'.
  private getDependencyKey(file: string, moduleInfo: ModuleInfo): string {
    const unixFilePath = file.replace(/\\/g, '/');
    return `${moduleInfo.packageName}/${unixFilePath}`;
  }

  private getTransformedDependencyKey(file: string, moduleInfo: ModuleInfo): string {
    const prefix = MODULE_SOURCE_ROOT.endsWith('/') ? MODULE_SOURCE_ROOT : `${MODULE_SOURCE_ROOT}/`;
    const unixFilePath = file.replace(/\\/g, '/');
    if (!unixFilePath.startsWith(prefix)) {
      return this.getDependencyKey(file, moduleInfo);
    }
    return `${moduleInfo.packageName}/${unixFilePath.substring(prefix.length)}`;
  }

  private isPackageEntryFile(file: string, moduleInfo: ModuleInfo): boolean {
    if (!moduleInfo.entryFile) {
      return false;
    }
    const entryRelPath = changeFileExtension(path.relative(moduleInfo.moduleRootPath, moduleInfo.entryFile), '')
      .replace(/\\/g, '/')
      .toLowerCase();
    const fileRelPath = changeFileExtension(file, '').replace(/\\/g, '/').toLowerCase();
    return fileRelPath === entryRelPath;
  }

  private parseDeclFile(
    consumerName: string,
    moduleInfo: ModuleInfo,
    dependencySection: Record<string, DependencyItem>
  ): void {
    if (!moduleInfo.declFilesPath || !fs.existsSync(moduleInfo.declFilesPath)) {
      // A static module without a declgen manifest is the common case; only the dynamic
      // and hybrid modules are expected to carry one. The self lookup (a 1.1 module
      // without interop declarations) is equally common and stays silent.
      if (moduleInfo.language !== LANGUAGE_VERSION.ARKTS_1_2 && consumerName !== moduleInfo.packageName) {
        logger.warn(
          `Module ${consumerName} depends on dynamic module ${moduleInfo.packageName}, ` +
          `but decl file not found on path ${moduleInfo.declFilesPath}`
        );
      }
      return;
    }
    let declFilesObject = JSON.parse(fs.readFileSync(moduleInfo.declFilesPath, 'utf-8'));
    Object.keys(declFilesObject.files).forEach((file: string) => {
      const fileEntry = declFilesObject.files[file];
      // Follows the build system: 'filePath' records the module source file behind the
      // declaration; older decl files only carry the module relative key.
      const sourceFilePath: string = fileEntry.filePath
        ? fileEntry.filePath
        : path.resolve(moduleInfo.moduleRootPath, `${file}.ets`);
      const dependencyItem: DependencyItem = {
        language: 'js',
        path: fileEntry.declPath,
        // Auto import maps definitions found in the source file back onto the module name.
        sourceFilePath,
        ohmUrl: fileEntry.ohmUrl
      };

      const legacyKey = this.getDependencyKey(file, moduleInfo);
      dependencySection[legacyKey] = dependencyItem;
      const transformedKey = this.getTransformedDependencyKey(file, moduleInfo);
      if (transformedKey !== legacyKey) {
        dependencySection[transformedKey] = dependencyItem;
      }

      if (this.isPackageEntryFile(file, moduleInfo)) {
        dependencySection[moduleInfo.packageName] = dependencyItem;
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
        this.traverse(dependencySection, basePath, aliasConfigObj, 'dynamic', true, '', false, ['.d.ets'], true, true);
      }
    });
  }

  private getDependenciesSection(moduleInfo: ModuleInfo, dependencySection: Record<string, DependencyItem>): void {
    this.generateSystemSdkDependenciesSection(dependencySection, moduleInfo);
    // Every dependency module contributes its declaration aliases when a decl-fileInfo
    // exists - a hybrid module appears in both lists, hence the deduplication.
    const depModules = Array.from(new Set([...moduleInfo.dynamicDepModuleInfos, ...moduleInfo.staticDepModuleInfos]));
    depModules.forEach((depModuleName: string) => {
      let depModuleInfo = this.moduleInfos[depModuleName];
      this.parseDeclFile(moduleInfo.packageName, depModuleInfo, dependencySection);
    });

    // The module's own decl-fileInfo is the materialized interop-declaration dynamic
    // entry list: hvigor generates it only for modules owning dynamic files and passes
    // those modules to the build system as 1.1/hybrid, whose own '<package>/<file>'
    // dependencies are then emitted. The locally computed language must not gate this:
    // the descriptor 'arktsversion' and the '.ets'-only file scan can both report '1.2'
    // for a module whose dynamic entries are declared through interop-declaration.json.
    this.parseDeclFile(moduleInfo.packageName, moduleInfo, dependencySection);
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
