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

import fs from 'node:fs';
import path from 'node:path';
import { Extension, Language, normalizePath } from './fileUtils';
import * as errors from './errors';

export interface FileManagerModuleInput {
  readonly packageName: string;
  readonly moduleName?: string;
  readonly modulePath: string;
  readonly language?: string;
  readonly packageVersion?: string;
  readonly declgenV1OutPath?: string;
  readonly declgenV2OutPath?: string;
  readonly staticFiles: readonly string[];
  readonly dynamicFiles: readonly string[];
}

export interface ModuleInfo {
  language: Language;
  moduleName: string;
  modulePath: string;
  packageName: string;
  packageVersion: string;
  dynamicFiles: Set<string>;
  staticFiles: Set<string>;
  declgenV1OutPath: string;
  declgenV2OutPath: string;
}

export enum Owner {
  MODULE,
  SDK,
  INTEROP_SDK,
}

export interface FileMeta {
  fileName: string;
  filePath: string;
  language: Language;
  owner: Owner;
  module?: ModuleInfo;
}

interface FileManagerInput {
  readonly staticStdLibPaths: readonly string[];
  readonly staticSdkPaths: readonly string[];
  readonly dynamicSdkPaths: readonly string[];
  readonly staticInteropSdkPaths: readonly string[];
  readonly dynamicInteropSdkPaths: readonly string[];
  readonly moduleList: readonly FileManagerModuleInput[];
}

export class FileManagerBuilder {
  private readonly staticStdLibPaths = new Set<string>();
  private readonly staticSdkPaths = new Set<string>();
  private readonly dynamicSdkPaths = new Set<string>();
  private readonly staticInteropSdkPaths = new Set<string>();
  private readonly dynamicInteropSdkPaths = new Set<string>();
  private readonly modulesByPackage = new Map<string, FileManagerModuleInput>();

  addStaticStdLibPath(stdLibPath: string): this {
    this.staticStdLibPaths.add(normalizePath(stdLibPath));
    return this;
  }

  addStaticSdkPath(sdkPath: string): this {
    this.staticSdkPaths.add(normalizePath(sdkPath));
    return this;
  }

  addStaticSdkPaths(sdkPaths: readonly string[]): this {
    for (const sdkPath of sdkPaths) {
      this.addStaticSdkPath(sdkPath);
    }
    return this;
  }

  addDynamicSdkPath(sdkPath: string): this {
    this.dynamicSdkPaths.add(normalizePath(sdkPath));
    return this;
  }

  addDynamicSdkPaths(sdkPaths: readonly string[]): this {
    for (const sdkPath of sdkPaths) {
      this.addDynamicSdkPath(sdkPath);
    }
    return this;
  }

  addStaticInteropSdkPath(sdkPath: string): this {
    this.staticInteropSdkPaths.add(normalizePath(sdkPath));
    return this;
  }

  addStaticInteropSdkPaths(sdkPaths: readonly string[]): this {
    for (const sdkPath of sdkPaths) {
      this.addStaticInteropSdkPath(sdkPath);
    }
    return this;
  }

  addDynamicInteropSdkPath(sdkPath: string): this {
    this.dynamicInteropSdkPaths.add(normalizePath(sdkPath));
    return this;
  }

  addDynamicInteropSdkPaths(sdkPaths: readonly string[]): this {
    for (const sdkPath of sdkPaths) {
      this.addDynamicInteropSdkPath(sdkPath);
    }
    return this;
  }

  addModule(module: FileManagerModuleInput): this {
    if (this.modulesByPackage.has(module.packageName)) {
      throw new Error(`Module "${module.packageName}" has already been added.`);
    }
    this.modulesByPackage.set(module.packageName, module);
    return this;
  }

  addModuleList(moduleList: readonly FileManagerModuleInput[]): this {
    for (const module of moduleList) {
      this.addModule(module);
    }
    return this;
  }

  build(): FileManager {
    return new BuiltFileManager({
      staticStdLibPaths: [...this.staticStdLibPaths],
      staticSdkPaths: [...this.staticSdkPaths],
      dynamicSdkPaths: [...this.dynamicSdkPaths],
      staticInteropSdkPaths: [...this.staticInteropSdkPaths],
      dynamicInteropSdkPaths: [...this.dynamicInteropSdkPaths],
      moduleList: [...this.modulesByPackage.values()],
    });
  }
}

export class FileManager {
  private staticStdLibFileSet: Set<string> = new Set();
  private staticSdkFileSet: Set<string> = new Set();
  private dynamicSdkFileSet: Set<string> = new Set();
  private staticInteropSdkFileSet: Set<string> = new Set();
  private dynamicInteropSdkFileSet: Set<string> = new Set();
  private staticSdkPathsByName: Map<string, string> = new Map();
  private dynamicSdkPathsByName: Map<string, string> = new Map();
  private staticInteropSdkPathsByName: Map<string, string> = new Map();
  private dynamicInteropSdkPathsByName: Map<string, string> = new Map();
  private staticSourceFileSet: Set<string> = new Set();
  private dynamicSourceFileSet: Set<string> = new Set();
  private pathToFileMetaMap: Map<string, FileMeta> = new Map();
  private packageToModuleInfoMap: Map<string, ModuleInfo> = new Map();
  private sourceToInteropDeclarationMap: Map<string, string> = new Map();
  private interopDeclarationToSourceMap: Map<string, string> = new Map();

  protected constructor(input: FileManagerInput) {
    this.staticStdLibFileSet = this.scanFiles(input.staticStdLibPaths);
    this.initStaticSdk(input.staticSdkPaths);
    this.initDynamicSdk(input.dynamicSdkPaths);
    this.initStaticInteropSdk(input.staticInteropSdkPaths);
    this.initDynamicInteropSdk(input.dynamicInteropSdkPaths);
    this.initModules(input.moduleList);
  }

  /** Static files owned by project modules and static SDK */
  get staticFiles(): ReadonlySet<string> {
    return this.staticSourceFileSet;
  }

  /** Dynamic files owned by project modules and dynamic SDK */
  get dynamicFiles(): ReadonlySet<string> {
    return this.dynamicSourceFileSet;
  }

  /** Static files owned by project modules, excluding SDK and interop SDK files. */
  get staticSourceFiles(): ReadonlySet<string> {
    return this.getModuleFiles(this.staticSourceFileSet);
  }

  /** Dynamic files owned by project modules, excluding SDK and interop SDK files. */
  get dynamicSourceFiles(): ReadonlySet<string> {
    return this.getModuleFiles(this.dynamicSourceFileSet);
  }

  isStaticStdLibFile(filePath: string): boolean {
    return this.staticStdLibFileSet.has(normalizePath(filePath));
  }

  isStaticSdkFile(filePath: string): boolean {
    return this.staticSdkFileSet.has(normalizePath(filePath));
  }

  isDynamicSdkFile(filePath: string): boolean {
    return this.dynamicSdkFileSet.has(normalizePath(filePath));
  }

  isDynamicInteropSdkFile(filePath: string): boolean {
    return this.dynamicInteropSdkFileSet.has(normalizePath(filePath));
  }

  isStaticInteropSdkFile(filePath: string): boolean {
    return this.staticInteropSdkFileSet.has(normalizePath(filePath));
  }

  isSdkFile(filePath: string): boolean {
    return (
      this.isStaticSdkFile(filePath) ||
      this.isDynamicSdkFile(filePath) ||
      this.isStaticInteropSdkFile(filePath) ||
      this.isDynamicInteropSdkFile(filePath)
    );
  }

  queryStaticSdkPath(sdkName: string): string | undefined {
    return this.staticSdkPathsByName.get(sdkName);
  }

  queryDynamicSdkPath(sdkName: string): string | undefined {
    return this.dynamicSdkPathsByName.get(sdkName);
  }

  /**
   * Path of the same-named declaration in the dynamic SDK for an SDK declaration
   * file, or undefined when the dynamic SDK has no counterpart. SDK names are
   * derived from the file basename (e.g. `@ohos.web.d.ets` -> `@ohos.web`).
   */
  queryDynamicSdkPathForFile(filePath: string): string | undefined {
    return this.dynamicSdkPathsByName.get(getSdkName(normalizePath(filePath)));
  }

  queryStaticInteropSdkPath(sdkName: string): string | undefined {
    return this.staticInteropSdkPathsByName.get(sdkName);
  }

  queryDynamicInteropSdkPath(sdkName: string): string | undefined {
    return this.dynamicInteropSdkPathsByName.get(sdkName);
  }

  isStaticSourceFile(filePath: string): boolean {
    const normalizedPath = normalizePath(filePath);
    return this.isModuleFile(normalizedPath) && this.staticSourceFileSet.has(normalizedPath);
  }

  isDynamicSourceFile(filePath: string): boolean {
    const normalizedPath = normalizePath(filePath);
    return this.isModuleFile(normalizedPath) && this.dynamicSourceFileSet.has(normalizedPath);
  }

  /** Whether a file is a static or dynamic source file owned by a project module. */
  isSourceFile(filePath: string): boolean {
    const normalizedPath = normalizePath(filePath);
    return (
      this.isModuleFile(normalizedPath) &&
      (this.staticSourceFileSet.has(normalizedPath) || this.dynamicSourceFileSet.has(normalizedPath))
    );
  }

  isStaticInteropFile(filePath: string): boolean {
    const sourceFile = this.interopDeclarationToSourceMap.get(normalizePath(filePath));
    return sourceFile !== undefined && this.staticSourceFileSet.has(sourceFile);
  }

  isDynamicInteropFile(filePath: string): boolean {
    const sourceFile = this.interopDeclarationToSourceMap.get(normalizePath(filePath));
    return sourceFile !== undefined && this.dynamicSourceFileSet.has(sourceFile);
  }

  queryFileMeta(filePath: string): FileMeta | undefined {
    return this.pathToFileMetaMap.get(normalizePath(filePath));
  }

  queryModuleInfo(packageName: string): ModuleInfo | undefined {
    return this.packageToModuleInfoMap.get(packageName);
  }

  queryInteropDeclarationFile(sourceFile: string): string | undefined {
    return this.sourceToInteropDeclarationMap.get(normalizePath(sourceFile));
  }

  querySourceFile(interopDeclarationFile: string): string | undefined {
    return this.interopDeclarationToSourceMap.get(normalizePath(interopDeclarationFile));
  }

  private isModuleFile(filePath: string): boolean {
    return this.pathToFileMetaMap.get(filePath)?.owner === Owner.MODULE;
  }

  private getModuleFiles(files: ReadonlySet<string>): ReadonlySet<string> {
    return new Set([...files].filter((file) => this.isModuleFile(file)));
  }

  private initStaticSdk(paths: readonly string[]): void {
    this.staticSdkFileSet = this.scanFiles(paths);
    this.registerSdkPaths(this.staticSdkFileSet, this.staticSdkPathsByName);
    this.registerFiles(this.staticSdkFileSet, Language.STATIC, Owner.SDK);
    mergeSet(this.staticSourceFileSet, this.staticSdkFileSet);
  }

  private initDynamicSdk(paths: readonly string[]): void {
    this.dynamicSdkFileSet = this.scanFiles(paths);
    this.registerSdkPaths(this.dynamicSdkFileSet, this.dynamicSdkPathsByName);
    this.registerFiles(this.dynamicSdkFileSet, Language.DYNAMIC, Owner.SDK);
    mergeSet(this.dynamicSourceFileSet, this.dynamicSdkFileSet);
  }

  private initStaticInteropSdk(paths: readonly string[]): void {
    this.staticInteropSdkFileSet = this.scanFiles(paths);
    this.registerSdkPaths(this.staticInteropSdkFileSet, this.staticInteropSdkPathsByName);
    this.registerFiles(this.staticInteropSdkFileSet, Language.DYNAMIC, Owner.INTEROP_SDK);
  }

  private initDynamicInteropSdk(paths: readonly string[]): void {
    this.dynamicInteropSdkFileSet = this.scanFiles(paths);
    this.registerSdkPaths(this.dynamicInteropSdkFileSet, this.dynamicInteropSdkPathsByName);
    this.registerFiles(this.dynamicInteropSdkFileSet, Language.STATIC, Owner.INTEROP_SDK);
  }

  private initModules(moduleList: readonly FileManagerModuleInput[]): void {
    for (const moduleConfig of moduleList) {
      const staticFiles = new Set(moduleConfig.staticFiles.map(normalizePath));
      const dynamicFiles = new Set(moduleConfig.dynamicFiles.map(normalizePath));
      const module = this.createModuleInfo(moduleConfig, staticFiles, dynamicFiles);
      this.packageToModuleInfoMap.set(module.packageName, module);
      for (const file of staticFiles) {
        this.staticSourceFileSet.add(file);
        this.setFileMeta(file, Language.STATIC, Owner.MODULE, module);
        this.registerInteropDeclaration(file, module.modulePath, module.declgenV1OutPath, '.d.ets');
      }
      for (const file of dynamicFiles) {
        this.dynamicSourceFileSet.add(file);
        this.setFileMeta(file, Language.DYNAMIC, Owner.MODULE, module);
        this.registerInteropDeclaration(file, module.modulePath, module.declgenV2OutPath, '.d.ets');
      }
    }
  }

  private registerInteropDeclaration(
    sourceFile: string,
    modulePath: string,
    declarationRoot: string,
    declarationSuffix: string,
  ): void {
    if (declarationRoot === '') {
      return;
    }
    const relativeSourcePath = path.relative(modulePath, sourceFile);
    const declarationFile = normalizePath(
      path.resolve(declarationRoot, `${stripSourceExtension(relativeSourcePath)}${declarationSuffix}`),
    );
    this.sourceToInteropDeclarationMap.set(sourceFile, declarationFile);
    this.interopDeclarationToSourceMap.set(declarationFile, sourceFile);
  }

  private createModuleInfo(
    moduleConfig: FileManagerModuleInput,
    staticFiles: Set<string>,
    dynamicFiles: Set<string>,
  ): ModuleInfo {
    return {
      language: moduleLanguage(moduleConfig, staticFiles, dynamicFiles),
      moduleName: moduleConfig.moduleName ?? moduleConfig.packageName,
      modulePath: normalizePath(moduleConfig.modulePath),
      packageName: moduleConfig.packageName,
      packageVersion: moduleConfig.packageVersion ?? '',
      dynamicFiles,
      staticFiles,
      declgenV1OutPath: normalizeOptionalPath(moduleConfig.declgenV1OutPath),
      declgenV2OutPath: normalizeOptionalPath(moduleConfig.declgenV2OutPath),
    };
  }

  private registerFiles(files: ReadonlySet<string>, language: Language, owner: Owner): void {
    for (const file of files) {
      this.setFileMeta(file, language, owner);
    }
  }

  private registerSdkPaths(files: ReadonlySet<string>, pathsByName: Map<string, string>): void {
    for (const filePath of files) {
      const sdkName = getSdkName(filePath);
      if (isPlatformSdkName(sdkName)) {
        pathsByName.set(sdkName, filePath);
      }
    }
  }

  private setFileMeta(filePath: string, language: Language, owner: Owner, module?: ModuleInfo): void {
    const normalizedPath = normalizePath(filePath);
    this.pathToFileMetaMap.set(normalizedPath, {
      fileName: path.basename(normalizedPath),
      filePath: normalizedPath,
      language,
      owner,
      ...(module === undefined ? {} : { module }),
    });
  }

  private scanFiles(rootPaths: readonly string[]): Set<string> {
    const files = new Set<string>();
    const pendingPaths = rootPaths.map(normalizePath);
    while (pendingPaths.length > 0) {
      const currentPath = pendingPaths.pop()!;
      const stat = fs.statSync(currentPath);
      if (stat.isDirectory()) {
        for (const entry of fs.readdirSync(currentPath)) {
          pendingPaths.push(normalizePath(path.join(currentPath, entry)));
        }
      } else if (stat.isFile() && isDeclarationFile(currentPath)) {
        files.add(normalizePath(currentPath));
      }
    }
    return files;
  }
}

class BuiltFileManager extends FileManager {
  constructor(input: FileManagerInput) {
    super(input);
  }
}

function moduleLanguage(
  moduleConfig: FileManagerModuleInput,
  staticFiles: ReadonlySet<string>,
  dynamicFiles: ReadonlySet<string>,
): Language {
  switch (moduleConfig.language) {
    case '1.1':
      return Language.DYNAMIC;
    case '1.2':
      return Language.STATIC;
    case 'hybrid':
      return Language.HYBRID;
    default:
      return staticFiles.size > 0 && dynamicFiles.size > 0
        ? Language.HYBRID
        : dynamicFiles.size > 0
          ? Language.DYNAMIC
          : Language.STATIC;
  }
}

function normalizeOptionalPath(filePath: string | undefined): string {
  return filePath === undefined || filePath === '' ? '' : normalizePath(filePath);
}

function stripSourceExtension(filePath: string): string {
  for (const extension of ['.d.ets', '.d.ts', '.ets', '.ts']) {
    if (filePath.endsWith(extension)) {
      return filePath.slice(0, -extension.length);
    }
  }
  return filePath;
}

function isDeclarationFile(filePath: string): boolean {
  return filePath.endsWith(Extension.DETS) || filePath.endsWith(Extension.DTS);
}

function getSdkName(filePath: string): string {
  const fileName = path.basename(filePath);
  return fileName.endsWith(Extension.DETS)
    ? fileName.slice(0, -Extension.DETS.length)
    : fileName.slice(0, -Extension.DTS.length);
}

function isPlatformSdkName(sdkName: string): boolean {
  return /^@(ohos|system|kit|arkts)\./i.test(sdkName);
}

function mergeSet<T>(target: Set<T>, source: Set<T>): void {
  for (const item of source) {
    target.add(item);
  }
}
