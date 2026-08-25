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

import { BuildConfig, AliasItem } from '../buildConfig';
import { DeclgenAdapter } from './declgenAdapter';
import * as common from '@interop-toolkits/common';
import * as utils from '../utils';
import * as dyn from '../dynamic';
import * as ts from 'typescript';
import * as predefined from '../predefine';
import * as fs from 'node:fs';
import path from 'node:path';

type ModuleResolutionResult = ts.ResolvedModuleFull | undefined;
type UiHandler = { createCustomTransformer: (node: ts.SourceFile) => ts.SourceFile };
const SDK_PREFIX = 'ohos|system|kit|arkts';

export class DynamicDeclgenAdapter extends DeclgenAdapter {
  private compilerOptions: ts.CompilerOptions;
  private cachePath: string;
  private moduleResolutionCache: Map<string, ModuleResolutionResult[]>;
  private sdkAliasMap: Map<string, Map<string, AliasItem>>;
  private libFilesPath: string;
  private readonly specifierResolutionCache = new Map<string, ModuleResolutionResult>();
  private readonly moduleResolutionHost: ts.ModuleResolutionHost;
  private readonly tsResolutionCache: ts.ModuleResolutionCache;
  private uiPlugin: dyn.DeclgenPlugin | undefined;

  constructor(buildConfig: BuildConfig, fileManager: common.fileManager.FileManager, compilerConfigPath: string) {
    super(buildConfig, fileManager, compilerConfigPath);
    this.compilerOptions = utils.loadTsCompilerOptions(compilerConfigPath);
    this.cachePath = path.join(buildConfig.cachePath, predefined.DECLGEN_DYN_CACHE_DIR_NAME);
    this.moduleResolutionCache = new Map();
    this.moduleResolutionHost = ts.sys;
    const getCanonicalFileName = ts.sys.useCaseSensitiveFileNames
      ? (fileName: string): string => fileName
      : (fileName: string): string => fileName.toLowerCase();
    this.tsResolutionCache = ts.createModuleResolutionCache(
      buildConfig.projectRootPath,
      getCanonicalFileName,
      this.compilerOptions,
    );
    this.sdkAliasMap = new Map(
      Object.entries(buildConfig.sdkAliasMap).map(([key, value]) => [key, new Map(Object.entries(value))]),
    );
    this.initUiPlugins();
    this.libFilesPath = path.join(buildConfig.buildDynamicSdkPath, predefined.UILIBS_FROM_DYN_SDK);
  }

  initUiPlugins(): void {
    const plugins = this.buildConfig.dynamicPlugins;
    const scripts = plugins['ArkUI-Interop'];
    const { HandleUIImports } = require(scripts);
    class UITraverser extends dyn.stages.Traverser<undefined, undefined> {
      uiHandler: unknown;
      constructor(
        context: ts.TransformationContext,
        typeChecker: ts.TypeChecker,
        state: dyn.stages.TraverserState<undefined, undefined>,
      ) {
        super(context, typeChecker, state);
        this.uiHandler = new HandleUIImports(typeChecker, context);
      }
      traverse(node: ts.SourceFile): ts.SourceFile {
        return (this.uiHandler as UiHandler).createCustomTransformer(node);
      }
    }
    class UIStage extends dyn.stages.TransformationStage<undefined, undefined, undefined> {
      override get name(): string {
        return 'ui-imports';
      }
      constructor() {
        super(
          [UITraverser],
          () => undefined,
          () => undefined,
          false,
        );
      }
    }
    this.uiPlugin = {
      name: 'ui-imports',
      stages: (): dyn.PluginStageSpec[] => [
        {
          id: 'com.ui-imports',
          version: '0.0.0', // After the plugin update, please update the version.
          stage: new UIStage(),
          dependencyKind: 'local-write',
          anchor: 'after-declaration',
          requiresFreshChecker: false,
        },
      ],
    };
  }

  getLibFiles(): string[] {
    const libFiles: string[] = [];
    fs.readdirSync(this.libFilesPath).forEach((file) => {
      if (/\.d\.ts$/.test(file)) {
        libFiles.push(common.fileUtils.normalizePath(path.resolve(this.libFilesPath, file)));
      }
    });
    return libFiles;
  }

  async run(entryFiles: string[]): Promise<void> {
    const declgenOptions: dyn.DeclgenOptions = {
      libFiles: this.getLibFiles(),
      inputFiles: entryFiles,
      rootDir: this.buildConfig.projectRootPath,
      features: {
        enableInteropTypesFix: true,
      },
      incremental: true,
      cacheDir: this.cachePath,
      verifyOutputs: true,
      plugins: [this.uiPlugin!],
    };

    dyn.logger.Logger.init(new dyn.logger.SilentLogger());

    const instance = new dyn.Declgen(declgenOptions, this.compilerOptions, this.resolveModuleNames.bind(this));
    const result = instance.run();
    result.emit((fileName, content) => {
      const outputPath = this.fileManager.queryInteropDeclarationFile(common.fileUtils.normalizePath(fileName));
      if (!outputPath) {
        throw new common.errors.InternalError(`Output path for ${fileName} not found in file manager.`);
      }
      const outputDir = path.dirname(outputPath);
      if (!fs.existsSync(outputDir)) {
        fs.mkdirSync(outputDir, { recursive: true });
      }
      fs.writeFileSync(outputPath, content, 'utf-8');
      return { artifactPath: outputPath };
    });
  }

  private getModuleResolutionCacheKey(moduleNames: string[], containingFile: string): string {
    return `${common.fileUtils.normalizePath(containingFile)}\x00${moduleNames.join('\x00')}`;
  }

  private resolveModuleNames(moduleNames: string[], containingFile: string): ModuleResolutionResult[] {
    const cacheKey = this.getModuleResolutionCacheKey(moduleNames, containingFile);
    if (this.moduleResolutionCache.has(cacheKey)) {
      return this.moduleResolutionCache.get(cacheKey)!;
    }
    const resolvedModules: ModuleResolutionResult[] = [];
    for (const moduleName of moduleNames) {
      const cacheKey = JSON.stringify([containingFile, moduleName]);
      if (this.specifierResolutionCache.has(cacheKey)) {
        resolvedModules.push(this.specifierResolutionCache.get(cacheKey)!);
        continue;
      }
      const resolved = this.resolveSpecifier(moduleName, containingFile);
      resolvedModules.push(resolved);
      this.specifierResolutionCache.set(cacheKey, resolved);
    }
    this.moduleResolutionCache.set(cacheKey, resolvedModules);
    return resolvedModules;
  }

  private resolveSpecifier(specifier: string, containingFile: string): ModuleResolutionResult {
    let resolved = this.resolveModuleNameFromDefault(specifier, containingFile);
    if (resolved !== undefined) {
      return resolved;
    }
    resolved = this.resolveModuleNameFromSdk(specifier);
    if (resolved !== undefined) {
      return resolved;
    }
    resolved = this.resolveModuleNameFromInteropSdk(specifier);
    if (resolved !== undefined) {
      return resolved;
    }
    resolved = this.resolveModuleNameFromSpecifierWithExtension(specifier, containingFile);
    if (resolved !== undefined) {
      return resolved;
    }
    if (this.sdkAliasMap) {
      resolved = this.resolveModuleNameFromSdkAlias(specifier, containingFile);
      if (resolved !== undefined) {
        return resolved;
      }
    }
    return undefined;
  }

  private resolveModuleNameFromDefault(specifier: string, containingFile: string): ts.ResolvedModuleFull | undefined {
    const resolved = ts.resolveModuleName(
      specifier,
      containingFile,
      this.compilerOptions,
      this.moduleResolutionHost,
      this.tsResolutionCache,
    );
    if (!resolved.resolvedModule) {
      return undefined;
    }
    const resolvedFileName = resolved.resolvedModule.resolvedFileName;
    if (path.extname(resolvedFileName) === common.fileUtils.Extension.JS) {
      const resultDETSPath = resolvedFileName.replace(common.fileUtils.Extension.JS, common.fileUtils.Extension.DETS);
      if (this.moduleResolutionHost.fileExists(resultDETSPath)) {
        return getResolveModule(resultDETSPath, common.fileUtils.Extension.DETS);
      }
    }
    return resolved.resolvedModule;
  }

  private resolveModuleNameFromSdk(specifier: string): ts.ResolvedModuleFull | undefined {
    const prefixRegex = new RegExp(`^@(${SDK_PREFIX})\\.`, 'i');
    if (!prefixRegex.test(specifier.trim())) {
      return undefined;
    }
    const resolvedPath = this.fileManager.queryDynamicSdkPath(specifier);
    if (resolvedPath === undefined) {
      return undefined;
    }
    const sdkExtension = getSdkExtension(resolvedPath);
    if (sdkExtension === undefined) {
      return undefined;
    }
    return getResolveModule(resolvedPath, sdkExtension);
  }

  private resolveModuleNameFromInteropSdk(specifier: string): ts.ResolvedModuleFull | undefined {
    const prefixRegex = new RegExp(`^static@(${SDK_PREFIX})\\.`, 'i');
    if (!prefixRegex.test(specifier.trim())) {
      return undefined;
    }
    const actualModuleName = specifier.replace(/^static@/i, '@');
    const resolvedPath = this.fileManager.queryStaticInteropSdkPath(actualModuleName);
    if (resolvedPath === undefined) {
      return undefined;
    }
    const sdkExtension = getSdkExtension(resolvedPath);
    if (sdkExtension === undefined) {
      return undefined;
    }
    return getResolveModule(resolvedPath, sdkExtension);
  }

  private resolveModuleNameFromSpecifierWithExtension(
    specifier: string,
    containingFile: string,
  ): ts.ResolvedModuleFull | undefined {
    if (/\.ts$/.test(specifier)) {
      const modulePath = path.resolve(path.dirname(containingFile), specifier);
      return this.moduleResolutionHost.fileExists(modulePath)
        ? getResolveModule(modulePath, common.fileUtils.Extension.TS)
        : undefined;
    }
    if (/\.ets$/.test(specifier) || /\.d\.ets$/.test(specifier)) {
      const modulePath = path.resolve(path.dirname(containingFile), specifier);
      return this.moduleResolutionHost.fileExists(modulePath)
        ? getResolveModule(modulePath, common.fileUtils.Extension.ETS)
        : undefined;
    }

    return undefined;
  }

  private resolveModuleNameFromSdkAlias(specifier: string, containingFile: string): ts.ResolvedModuleFull | undefined {
    if (!this.sdkAliasMap) {
      return undefined;
    }
    const aliasMap = this.sdkAliasMap!;
    const fileMeta = this.fileManager.queryFileMeta(containingFile);
    if (!fileMeta) {
      return undefined;
    }
    const packageName = fileMeta.module?.packageName;
    if (!packageName) {
      return undefined;
    }
    const packageAliasMap = aliasMap.get(packageName);
    if (!packageAliasMap) {
      return undefined;
    }
    const aliasItem = packageAliasMap.get(specifier);
    if (!aliasItem || !aliasItem.isStatic) {
      return undefined;
    }
    return this.resolveModuleNameFromInteropSdk(aliasItem.originalAPIName);
  }
}

function getResolveModule(modulePath: string, type: common.fileUtils.Extension): ts.ResolvedModuleFull {
  return {
    resolvedFileName: modulePath,
    isExternalLibraryImport: false,
    extension: type as string as ts.Extension,
  };
}

function getSdkExtension(modulePath: string): common.fileUtils.Extension | undefined {
  if (modulePath.endsWith(common.fileUtils.Extension.DETS)) {
    return common.fileUtils.Extension.DETS;
  }
  if (modulePath.endsWith(common.fileUtils.Extension.DTS)) {
    return common.fileUtils.Extension.DTS;
  }
  return undefined;
}
