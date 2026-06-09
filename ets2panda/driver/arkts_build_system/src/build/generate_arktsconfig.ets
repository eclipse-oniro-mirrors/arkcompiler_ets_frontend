/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import {
  AliasConfig,
  CompilerOptions,
  DependencyItem,
  ArkTSConfigObject,
  BuildConfig,
  ModuleInfo,
} from '../types';
import {
  LANGUAGE_VERSION,
} from '../pre_define';
import {
  Logger,
  LogDataFactory
} from '../logger';
import {
  ErrorCode,
  DriverError
} from '../util/error';

export interface AddDependencyArg {
  name: string;
  item: DependencyItem;
}

export class ArkTSConfig {
  private _object: ArkTSConfigObject;

  constructor(moduleInfo: ModuleInfo, cacheDir: string, declgenV2OutPath: string) {
    this._object = {
      compilerOptions: {
        packageName: moduleInfo.packageName,
        baseUrl: moduleInfo.moduleRootPath,
        rootDir: moduleInfo.moduleRootPath,
        paths: new Map<string, string[]>(),
        dependencies: new Map<string, DependencyItem>(),
        cacheDir: cacheDir,
        declgenV2OutPath: declgenV2OutPath,
      }
    };
  }

  addPathMappings(mappings: Map<string, string[]>): void {
    const paths = this._object.compilerOptions.paths;
    for (const entry of mappings.entries()) {
      const key = entry[0];
      const values = entry[1];
      const current = paths.get(key);
      if (current === undefined) {
        paths.set(key, values);
      } else {
        const merged: string[] = [];
        for (const value of current) {
          if (merged.indexOf(value) < 0) {
            merged.push(value);
          }
        }
        for (const value of values) {
          if (merged.indexOf(value) < 0) {
            merged.push(value);
          }
        }
        paths.set(key, merged);
      }
    }
  }

  addDependency(dep: AddDependencyArg): void {
    const deps = this._object.compilerOptions.dependencies;
    const existing = deps.get(dep.name);
    if (existing !== undefined) {
      const mergedAlias: string[] = [];
      const existingAlias = existing.alias;
      if (existingAlias !== undefined) {
        for (const aliasName of existingAlias) {
          if (mergedAlias.indexOf(aliasName) < 0) {
            mergedAlias.push(aliasName);
          }
        }
      }
      const depAlias = dep.item.alias;
      if (depAlias !== undefined) {
        for (const aliasName of depAlias) {
          if (mergedAlias.indexOf(aliasName) < 0) {
            mergedAlias.push(aliasName);
          }
        }
      }
      deps.set(dep.name, {
        language: dep.item.language,
        path: dep.item.path,
        ohmUrl: dep.item.ohmUrl,
        sourceFilePath: dep.item.sourceFilePath,
        alias: mergedAlias,
        mainFile: dep.item.mainFile !== undefined ? dep.item.mainFile : existing.mainFile,
      } as DependencyItem);
    } else {
      deps.set(dep.name, dep.item);
    }
  }

  addDependencies(deps: Map<string, DependencyItem>): void {
    for (const entry of deps.entries()) {
      this.addDependency({ name: entry[0], item: entry[1] });
    }
  }

  mergeArktsConfig(source: ArkTSConfig | undefined): void {
    if (source === undefined) {
      return;
    }
    this.addDependencies(source.dependencies);
    this.addPathMappings(source.pathSection);
  }

  mergeArktsConfigByDependencies(dependencies: Set<string>, dependenciesSets: Map<string, Set<string>>,
    generator: ArkTSConfigGenerator): void {
    if (dependencies.size === 0) {
      return;
    }
    const processed: Set<string> = new Set<string>();
    for (const dependency of dependencies) {
      this.mergeDependencyRecursive(dependency, dependenciesSets, processed, generator);
    }
  }

  private mergeDependencyRecursive(dependency: string, dependenciesSets: Map<string, Set<string>>,
    processed: Set<string>, generator: ArkTSConfigGenerator): void {
    if (processed.has(dependency)) {
      return;
    }
    processed.add(dependency);
    const depDeps = dependenciesSets.get(dependency);
    if (depDeps !== undefined && depDeps.size > 0) {
      for (const dep of depDeps) {
        this.mergeDependencyRecursive(dep, dependenciesSets, processed, generator);
      }
    }
    const arktsConfig = generator.getArktsConfigByPackageName(dependency);
    if (arktsConfig === undefined) {
      return;
    }
    this.addDependencies(arktsConfig.dependencies);
    this.addPathMappings(arktsConfig.pathSection);
  }

  public get compilerOptions(): CompilerOptions {
    return this._object.compilerOptions;
  }

  public get packageName(): string {
    return this._object.compilerOptions.packageName;
  }

  public get dependencies(): Map<string, DependencyItem> {
    return this._object.compilerOptions.dependencies;
  }

  public get pathSection(): Map<string, string[]> {
    return this._object.compilerOptions.paths;
  }

  public set useEmptyPackage(value: boolean) {
    this._object.compilerOptions.useEmptyPackage = value;
  }
}

const ETS_STDLIB_PACKAGES: string[] = [
  'std/core',
  'std/math',
  'std/math/consts',
  'std/containers',
  'std/interop/js',
  'std/time',
  'std/debug',
  'std/debug/concurrency',
  'std/dfx',
  'std/testing',
  'std/concurrency',
  'std/annotations',
  'std/interop',
  'escompat',
  'arkruntime',
];

export class ArkTSConfigGenerator {
  private buildConfig: BuildConfig;
  private stdlibAbcPath: string;
  private arktsconfigs: Map<string, ArkTSConfig>;
  private logger: Logger;

  constructor(buildConfig: BuildConfig) {
    this.logger = Logger.getInstance();
    this.buildConfig = buildConfig;
    const pandaSdkPath = buildConfig.pandaSdkPath ?? '';
    this.stdlibAbcPath = normalizePath(pandaSdkPath + '/lib/etsstdlib.abc');
    this.arktsconfigs = new Map<string, ArkTSConfig>();
  }

  public get aliasConfig(): Map<string, Map<string, AliasConfig>> {
    return this.buildConfig.aliasConfig;
  }

  public get dynamicSDKPaths(): Set<string> {
    return this.buildConfig.interopSDKPaths;
  }

  public get externalApiPaths(): string[] {
    return this.buildConfig.externalApiPaths;
  }

  public generateArkTSConfigFile(moduleInfo: ModuleInfo, enableDeclgenEts2Ts: boolean): ArkTSConfig {
    if (moduleInfo.sourceRoots.length === 0) {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_SOURCEROOTS_NOT_SET_FAIL,
        'SourceRoots not set for module ' + moduleInfo.packageName));
    }
    const arktsConfig = new ArkTSConfig(moduleInfo, this.buildConfig.cachePath,
      this.buildConfig.declgenV2OutPath);
    this.arktsconfigs.set(moduleInfo.packageName, arktsConfig);

    this.addPathSection(moduleInfo, arktsConfig);
    this.addEtsStdLibToDependencySection(arktsConfig);

    this.logger.printDebug('Generated arktsconfig for ' + moduleInfo.packageName);
    return arktsConfig;
  }

  private addPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    if (moduleInfo.language !== undefined && moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1.valueOf()) {
      return;
    }
    this.addSourceRootPaths(moduleInfo, arktsConfig);
  }

  private addSourceRootPaths(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    if (moduleInfo.packageName.length === 0) {
      return;
    }
    const paths: string[] = [];
    for (let i = moduleInfo.sourceRoots.length - 1; i >= 0; i--) {
      paths.push(normalizePath(moduleInfo.moduleRootPath + '/' + moduleInfo.sourceRoots[i]));
    }
    paths.push(normalizePath(moduleInfo.moduleRootPath + '/'));
    const packagePaths = new Map<string, string[]>();
    packagePaths.set(moduleInfo.packageName, paths);
    arktsConfig.addPathMappings(packagePaths);

    if (moduleInfo.entryFile.length > 0) {
      const entryFilePath = normalizePath(moduleInfo.moduleRootPath + '/' + moduleInfo.entryFile);
      const entryPaths = new Map<string, string[]>();
      entryPaths.set(moduleInfo.packageName + '/Index', [entryFilePath]);
      arktsConfig.addPathMappings(entryPaths);
    }
  }

  private addEtsStdLibToDependencySection(arktsconfig: ArkTSConfig): void {
    for (const currentName of ETS_STDLIB_PACKAGES) {
      const depItem: DependencyItem = {
        language: 'ets',
        path: this.stdlibAbcPath,
        ohmUrl: currentName,
        sourceFilePath: undefined,
        alias: undefined,
        mainFile: undefined,
      };
      arktsconfig.addDependency({ name: currentName, item: depItem });
    }
  }

  public getArktsConfigByPackageName(packageName: string): ArkTSConfig | undefined {
    return this.arktsconfigs.get(packageName);
  }

  public setArktsConfigByPackageName(packageName: string, config: ArkTSConfig): void {
    this.arktsconfigs.set(packageName, config);
  }
}

function stripExtension(filePath: string): string {
  const lastSlash = filePath.lastIndexOf('/');
  const fileName = lastSlash >= 0 ? filePath.substring(lastSlash + 1) : filePath;
  const dotIdx = fileName.indexOf('.');
  if (dotIdx >= 0) {
    return lastSlash >= 0 ? filePath.substring(0, lastSlash + 1) + fileName.substring(0, dotIdx) : fileName.substring(0, dotIdx);
  }
  return filePath;
}

function normalizePath(p: string): string {
  const segments = p.split('/');
  const normalized: string[] = [];
  for (const segment of segments) {
    if (segment === '' || segment === '.') {
      continue;
    }
    if (segment === '..') {
      if (normalized.length > 0) {
        normalized.pop();
      }
      continue;
    }
    normalized.push(segment);
  }
  const prefix = p.startsWith('/') ? '/' : '';
  return prefix + normalized.join('/');
}
