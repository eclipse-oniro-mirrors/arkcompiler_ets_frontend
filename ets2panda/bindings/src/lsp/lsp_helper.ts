/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { LspDriverHelper } from '../common/driver_helper';
import { global } from '../common/global';
import {
  LspDefinitionData,
  LspDiagsNode,
  LspReferences,
  LspQuickInfo,
  LspClassHierarchy,
  LspCompletionEntryKind,
  LspClassPropertyInfo,
  LspClassHierarchies,
  LspDocumentHighlightsReferences,
  LspCompletionInfo,
  LspReferenceLocationList,
  LspLineAndCharacter,
  LspReferenceData,
  LspClassConstructorInfo,
  ApplicableRefactorItemInfo,
  LspApplicableRefactorInfo,
  CompletionEntryDetails,
  LspFileTextChanges,
  LspSafeDeleteLocationInfo,
  LspSafeDeleteLocation,
  LspTypeHierarchiesInfo,
  LspTextSpan,
  LspInlayHint,
  LspInlayHintList,
  TextSpan,
  LspSignatureHelpItems,
  CodeFixActionInfo,
  CodeFixActionInfoList,
  LspRenameLocation,
  LspRenameInfoType,
  LspRenameInfoSuccess,
  LspRenameInfoFailure,
  LspSourceLocation,
  LspNodeInfo
} from './lspNode';
import { passStringArray, unpackString } from '../common/private';
import { Es2pandaContextState } from '../generated/Es2pandaEnums';
import {
  BuildConfig,
  Config,
  FileDepsInfo,
  Job,
  JobInfo,
  WorkerInfo,
  ModuleInfo,
  PathConfig,
  TextDocumentChangeInfo,
  NodeInfo,
  AstNodeType
} from '../common/types';
import { PluginDriver, PluginHook } from '../common/ui_plugins_driver';
import { ModuleDescriptor, generateBuildConfigs } from './generateBuildConfig';
import { generateArkTsConfigs, generateModuleInfo } from './generateArkTSConfig';

import * as fs from 'fs';
import * as path from 'path';
import { KInt, KNativePointer, KPointer } from '../common/InteropTypes';
import { passPointerArray } from '../common/private';
import { NativePtrDecoder } from '../common/Platform';
import { Worker as ThreadWorker } from 'worker_threads';
import { ensurePathExists, getFileLanguageVersion } from '../common/utils';
import * as child_process from 'child_process';
import { DECL_ETS_SUFFIX, DEFAULT_CACHE_DIR, LANGUAGE_VERSION, TS_SUFFIX } from '../common/preDefine';
import * as crypto from 'crypto';
import * as os from 'os';
import { changeDeclgenFileExtension } from '../common/utils';

const ets2pandaCmdPrefix = ['-', '--extension', 'ets', '--arktsconfig'];

function initBuildEnv(): void {
  const currentPath: string | undefined = process.env.PATH;
  let pandaLibPath: string = process.env.PANDA_LIB_PATH
    ? process.env.PANDA_LIB_PATH
    : path.resolve(__dirname, '../../../ets2panda/lib');
  process.env.PATH = `${currentPath}${path.delimiter}${pandaLibPath}`;
}

export class Lsp {
  private pandaLibPath: string;
  private pandaBinPath: string;
  private getFileContent: (filePath: string) => string;
  private filesMap: Map<string, string>; // Map<fileName, fileContent>
  private cacheDir: string;
  private globalContextPtr?: KNativePointer;
  private globalConfig?: Config;
  private globalLspDriverHelper?: LspDriverHelper;
  private defaultArkTsConfig: string;
  private defaultBuildConfig: BuildConfig;
  private fileDependencies: string;
  private buildConfigs: Record<string, BuildConfig>; // Map<moduleName, build_config.json>
  private moduleInfos: Record<string, ModuleInfo>; // Map<fileName, ModuleInfo>
  private pathConfig: PathConfig;
  private lspDriverHelper = new LspDriverHelper();
  private declFileMap: Record<string, string> = {}; // Map<declFilePath, sourceFilePath>

  constructor(pathConfig: PathConfig, getContentCallback?: (filePath: string) => string, modules?: ModuleDescriptor[]) {
    initBuildEnv();
    this.cacheDir =
      pathConfig.cacheDir !== undefined ? pathConfig.cacheDir : path.join(pathConfig.projectPath, DEFAULT_CACHE_DIR);
    this.fileDependencies = path.join(this.cacheDir, 'file_dependencies.json');
    this.pandaLibPath = process.env.PANDA_LIB_PATH
      ? process.env.PANDA_LIB_PATH
      : path.resolve(__dirname, '../../../ets2panda/lib');
    this.pandaBinPath = process.env.PANDA_BIN_PATH
      ? process.env.PANDA_BIN_PATH
      : path.resolve(__dirname, '../../../ets2panda/bin');
    this.filesMap = new Map<string, string>();
    this.getFileContent = getContentCallback || ((path: string): string => fs.readFileSync(path, 'utf8'));
    this.buildConfigs = generateBuildConfigs(pathConfig, modules);
    this.moduleInfos = generateArkTsConfigs(this.buildConfigs);
    this.pathConfig = pathConfig;
    this.defaultArkTsConfig = Object.values(this.moduleInfos)[0].arktsConfigFile;
    this.defaultBuildConfig = Object.values(this.buildConfigs)[0];
    PluginDriver.getInstance().initPlugins(this.defaultBuildConfig);
    this.initDeclFile();
  }

  // Partially update for new file
  updateModuleInfos(module: ModuleDescriptor, newFilePath: String): void {
    let buildConfig = this.buildConfigs[module.name];
    buildConfig.compileFiles.push(newFilePath.valueOf());
    let moduleInfo = generateModuleInfo(this.buildConfigs, buildConfig);
    this.moduleInfos[newFilePath.valueOf()] = moduleInfo;
  }

  // Full update for `Sync Now`
  update(modules: ModuleDescriptor[]): void {
    this.buildConfigs = generateBuildConfigs(this.pathConfig, modules);
    this.moduleInfos = generateArkTsConfigs(this.buildConfigs);
  }

  modifyFilesMap(fileName: string, fileContent: TextDocumentChangeInfo): void {
    this.filesMap.set(fileName, fileContent.newDoc);
  }

  deleteFromFilesMap(fileName: string): void {
    this.filesMap.delete(fileName);
  }

  private getFileSource(filePath: string): string {
    const getSource = this.filesMap.get(filePath) || this.getFileContent(filePath) || fs.readFileSync(filePath, 'utf8');
    if (getSource === undefined) {
      throw new Error(`File content not found for path: ${filePath}`);
    }
    return getSource.replace(/\r\n/g, '\n');
  }

  private createContext(filename: String, processToCheck: boolean = true): [Config, KNativePointer] {
    const filePath = path.resolve(filename.valueOf());
    const arktsconfig = Object.prototype.hasOwnProperty.call(this.moduleInfos, filePath)
      ? this.moduleInfos[filePath].arktsConfigFile
      : this.defaultArkTsConfig;
    if (!arktsconfig) {
      throw new Error(`Missing arktsconfig for ${filePath}`);
    }

    const ets2pandaCmd = [...ets2pandaCmdPrefix, arktsconfig];
    const localCfg = this.lspDriverHelper.createCfg(ets2pandaCmd, filePath, this.pandaLibPath);
    const source = this.getFileSource(filePath);

    const localCtx = this.lspDriverHelper.createCtx(source, filePath, localCfg, this.globalContextPtr);
    try {
      const packageName = Object.prototype.hasOwnProperty.call(this.moduleInfos, filePath)
        ? this.moduleInfos[filePath].packageName
        : undefined;
      const buildConfig = packageName ? this.buildConfigs[packageName] : this.defaultBuildConfig;
      const pluginContext = PluginDriver.getInstance().getPluginContext();
      pluginContext.setCodingFilePath(filePath);
      pluginContext.setProjectConfig(buildConfig);
      pluginContext.setContextPtr(localCtx);

      this.lspDriverHelper.proceedToState(localCtx, Es2pandaContextState.ES2PANDA_STATE_PARSED);
      PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

      if (processToCheck) {
        this.lspDriverHelper.proceedToState(localCtx, Es2pandaContextState.ES2PANDA_STATE_CHECKED);
      }
      return [localCfg, localCtx];
    } catch (error) {
      this.lspDriverHelper.destroyContext(localCtx);
      this.lspDriverHelper.destroyConfig(localCfg);
      throw error;
    }
  }

  private destroyContext(config: Config, context: KNativePointer): void {
    try {
      PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    } finally {
      this.lspDriverHelper.destroyContext(context);
      this.lspDriverHelper.destroyConfig(config);
    }
  }

  private generateDeclFile(filePath: string): void {
    const fileSource = this.getFileSource(filePath);
    if (getFileLanguageVersion(fileSource) === LANGUAGE_VERSION.ARKTS_1_2) {
      const [cfg, ctx] = this.createContext(filePath);
      try {
        let moduleInfo = this.moduleInfos[filePath];
        let modulePath: string = path.relative(moduleInfo.moduleRootPath, filePath);
        let declEtsOutputPath: string = changeDeclgenFileExtension(
          path.join(moduleInfo.declgenV1OutPath!, modulePath),
          DECL_ETS_SUFFIX
        );
        let etsOutputPath: string = changeDeclgenFileExtension(
          path.join(moduleInfo.declgenBridgeCodePath!, modulePath),
          TS_SUFFIX
        );
        this.declFileMap[declEtsOutputPath] = filePath;
        ensurePathExists(declEtsOutputPath);
        ensurePathExists(etsOutputPath);
        global.es2pandaPublic._GenerateTsDeclarationsFromContext(ctx, declEtsOutputPath, etsOutputPath, 1, 0, '');
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
  }

  private initDeclFile(): void {
    for (const filePath of Object.keys(this.moduleInfos)) {
      this.generateDeclFile(filePath);
    }
  }

  updateDeclFile(filePath: string): void {
    if (!Object.prototype.hasOwnProperty.call(this.moduleInfos, filePath)) {
      return;
    }
    this.generateDeclFile(filePath);
  }

  getOffsetByColAndLine(filename: String, line: number, column: number): number {
    const sourceCode = this.getFileSource(filename.valueOf());
    return global.es2panda._getOffsetByColAndLine(sourceCode, line, column);
  }

  getColAndLineByOffset(filename: String, offset: number): LspSourceLocation {
    const sourceCode = this.getFileSource(filename.valueOf());
    return new LspSourceLocation(global.es2panda._getColAndLineByOffset(sourceCode, offset));
  }

  getDefinitionAtPosition(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspDefinitionData {
    if (nodeInfos) {
      return this.getDefinitionAtPositionByNodeInfos(filename, nodeInfos);
    }
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getDefinitionAtPosition(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    const result = new LspDefinitionData(ptr);
    const moduleName = this.moduleInfos[filename.valueOf()].packageName;
    const declgenOutDir = this.buildConfigs[moduleName].declgenOutDir;
    if (result.fileName.endsWith(DECL_ETS_SUFFIX) && result.fileName.startsWith(declgenOutDir)) {
      let ptr: KPointer;
      const [declFileCfg, declFileCtx] = this.createContext(result.fileName, false);
      try {
        ptr = global.es2panda._getNodeInfosByDefinitionData(declFileCtx, result.start);
        result.nodeInfos = new NativePtrDecoder().decode(ptr).map((elPeer: KNativePointer) => {
          return new LspNodeInfo(elPeer);
        });
      } finally {
        this.destroyContext(declFileCfg, declFileCtx);
      }
    }
    return result;
  }

  private getDefinitionAtPositionByNodeInfos(declFilePath: String, nodeInfos: NodeInfo[]): LspDefinitionData {
    let ptr: KPointer;
    let nodeInfoPtrs: KPointer[] = [];
    const sourceFilePath = this.declFileMap[declFilePath.valueOf()];
    const [cfg, ctx] = this.createContext(sourceFilePath, false);
    try {
      nodeInfos.forEach((nodeInfo) => {
        nodeInfoPtrs.push(global.es2panda._CreateNodeInfoPtr(nodeInfo.name, nodeInfo.kind));
      });
      ptr = global.es2panda._getDefinitionDataFromNode(ctx, passPointerArray(nodeInfoPtrs), nodeInfoPtrs.length);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDefinitionData(ptr, sourceFilePath);
  }

  private getMergedCompileFiles(filename: String): string[] {
    const moduleInfo = this.moduleInfos[path.resolve(filename.valueOf())];
    return moduleInfo ? [...moduleInfo.compileFiles, ...moduleInfo.depModuleCompileFiles] : [];
  }

  getSemanticDiagnostics(filename: String): LspDiagsNode {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getSemanticDiagnostics(ctx);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDiagsNode(ptr);
  }

  getCurrentTokenValue(filename: String, offset: number): string {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getCurrentTokenValue(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return unpackString(ptr);
  }

  getImplementationAtPosition(filename: String, offset: number): LspDefinitionData {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getImplementationAtPosition(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDefinitionData(ptr);
  }

  getFileReferences(filename: String): LspReferenceData[] {
    let isPackageModule: boolean;
    const [cfg, searchCtx] = this.createContext(filename);
    try {
      isPackageModule = global.es2panda._isPackageModule(searchCtx);
    } finally {
      this.destroyContext(cfg, searchCtx);
    }
    let result: LspReferenceData[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      const [cfg, ctx] = this.createContext(compileFiles[i]);
      try {
        ptr = global.es2panda._getFileReferences(path.resolve(filename.valueOf()), ctx, isPackageModule);
      } finally {
        this.destroyContext(cfg, ctx);
      }
      let refs = new LspReferences(ptr);
      for (let j = 0; j < refs.referenceInfos.length; j++) {
        if (refs.referenceInfos[j].fileName !== '') {
          result.push(refs.referenceInfos[j]);
        }
      }
    }
    return result;
  }

  getReferencesAtPosition(filename: String, offset: number): LspReferenceData[] {
    let declInfo: KPointer;
    const [cfg, searchCtx] = this.createContext(filename);
    try {
      declInfo = global.es2panda._getDeclInfo(searchCtx, offset);
    } finally {
      this.destroyContext(cfg, searchCtx);
    }
    let result: LspReferenceData[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      const [cfg, ctx] = this.createContext(compileFiles[i]);
      try {
        ptr = global.es2panda._getReferencesAtPosition(ctx, declInfo);
      } finally {
        this.destroyContext(cfg, ctx);
      }
      let refs = new LspReferences(ptr);
      result.push(...refs.referenceInfos);
    }
    return Array.from(new Set(result));
  }

  getTypeHierarchies(filename: String, offset: number): LspTypeHierarchiesInfo | null {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    ptr = global.es2panda._getTypeHierarchies(ctx, ctx, offset);
    let ref = new LspTypeHierarchiesInfo(ptr);
    if (ref.fileName === '') {
      this.destroyContext(cfg, ctx);
      return null;
    }
    let result: LspTypeHierarchiesInfo[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let searchPtr: KPointer;
      const [cfg, searchCtx] = this.createContext(compileFiles[i]);
      try {
        searchPtr = global.es2panda._getTypeHierarchies(searchCtx, ctx, offset);
      } finally {
        this.destroyContext(cfg, searchCtx);
      }
      let refs = new LspTypeHierarchiesInfo(searchPtr);
      if (i > 0) {
        result[0].subHierarchies.subOrSuper = result[0].subHierarchies.subOrSuper.concat(
          refs.subHierarchies.subOrSuper
        );
      } else {
        result.push(refs);
      }
    }
    for (let j = 0; j < result[0].subHierarchies.subOrSuper.length; j++) {
      let res = this.getTypeHierarchies(
        result[0].subHierarchies.subOrSuper[j].fileName,
        result[0].subHierarchies.subOrSuper[j].pos
      );
      if (res !== null) {
        let subOrSuperTmp = result[0].subHierarchies.subOrSuper[j].subOrSuper.concat(res.subHierarchies.subOrSuper);
        result[0].subHierarchies.subOrSuper[j].subOrSuper = Array.from(
          new Map(
            subOrSuperTmp.map((item) => [`${item.fileName}-${item.type}-${item.pos}-${item.name}`, item])
          ).values()
        );
      }
    }
    return result[0];
  }

  getClassHierarchyInfo(filename: String, offset: number): LspClassHierarchy {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getClassHierarchyInfo(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspClassHierarchy(ptr);
  }

  getAliasScriptElementKind(filename: String, offset: number): LspCompletionEntryKind {
    let kind: KInt;
    const [cfg, ctx] = this.createContext(filename);
    try {
      kind = global.es2panda._getAliasScriptElementKind(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return kind;
  }

  getClassHierarchies(filename: String, offset: number): LspClassHierarchies {
    let contextList = [];
    const [cfg, ctx] = this.createContext(filename);
    contextList.push({ ctx: ctx, cfg: cfg });
    let nativeContextList = global.es2panda._pushBackToNativeContextVector(ctx, ctx, 1);
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let filePath = path.resolve(compileFiles[i]);
      if (path.resolve(filename.valueOf()) === filePath) {
        continue;
      }
      const [searchCfg, searchCtx] = this.createContext(filePath);
      contextList.push({ ctx: searchCtx, cfg: searchCfg });
      global.es2panda._pushBackToNativeContextVector(searchCtx, nativeContextList, 0);
    }
    let ptr = global.es2panda._getClassHierarchies(nativeContextList, filename, offset);
    PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    for (const { ctx, cfg } of contextList) {
      this.destroyContext(cfg, ctx);
    }
    return new LspClassHierarchies(ptr);
  }

  getClassPropertyInfo(
    filename: String,
    offset: number,
    shouldCollectInherited: boolean = false
  ): LspClassPropertyInfo {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getClassPropertyInfo(ctx, offset, shouldCollectInherited);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspClassPropertyInfo(ptr);
  }

  getOrganizeImports(filename: String): LspFileTextChanges {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._organizeImports(ctx, filename);
      PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspFileTextChanges(ptr);
  }

  findSafeDeleteLocation(filename: String, offset: number): LspSafeDeleteLocationInfo[] {
    let declInfo: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      declInfo = global.es2panda._getDeclInfo(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    let result: LspSafeDeleteLocationInfo[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      const [searchCfg, searchCtx] = this.createContext(compileFiles[i]);
      try {
        ptr = global.es2panda._findSafeDeleteLocation(searchCtx, declInfo);
      } finally {
        this.destroyContext(searchCfg, searchCtx);
      }
      let refs = new LspSafeDeleteLocation(ptr);
      result.push(...refs.safeDeleteLocationInfos);
    }
    return Array.from(new Set(result));
  }

  getCompletionEntryDetails(filename: String, offset: number, entryName: String): CompletionEntryDetails {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getCompletionEntryDetails(entryName, filename, ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new CompletionEntryDetails(ptr);
  }

  getApplicableRefactors(filename: String, kind: String, offset: number): ApplicableRefactorItemInfo[] {
    let ptr: KPointer;
    let result: ApplicableRefactorItemInfo[] = [];
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getApplicableRefactors(ctx, kind, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    let refs = new LspApplicableRefactorInfo(ptr);
    result.push(...refs.applicableRefactorInfo);
    return Array.from(new Set(result));
  }

  getClassConstructorInfo(filename: String, offset: number, properties: string[]): LspClassConstructorInfo {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getClassConstructorInfo(ctx, offset, passStringArray(properties));
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspClassConstructorInfo(ptr);
  }

  getSyntacticDiagnostics(filename: String): LspDiagsNode {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename, false);
    try {
      ptr = global.es2panda._getSyntacticDiagnostics(ctx);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDiagsNode(ptr);
  }

  getSuggestionDiagnostics(filename: String): LspDiagsNode {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getSuggestionDiagnostics(ctx);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDiagsNode(ptr);
  }

  getQuickInfoAtPosition(filename: String, offset: number): LspQuickInfo {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getQuickInfoAtPosition(filename, ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspQuickInfo(ptr);
  }

  getDocumentHighlights(filename: String, offset: number): LspDocumentHighlightsReferences {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getDocumentHighlights(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspDocumentHighlightsReferences(ptr);
  }

  getCompletionAtPosition(filename: String, offset: number): LspCompletionInfo {
    let lspDriverHelper = new LspDriverHelper();
    let filePath = path.resolve(filename.valueOf());
    let arktsconfig = this.moduleInfos[filePath].arktsConfigFile;
    let ets2pandaCmd = ets2pandaCmdPrefix.concat(arktsconfig);
    let localCfg = lspDriverHelper.createCfg(ets2pandaCmd, filePath, this.pandaLibPath);
    let source = this.getFileSource(filePath);
    // This is a temporary solution to support "obj." with wildcard for better solution in internal issue.
    if (source[offset - 1] === '.') {
      const wildcard = '_WILDCARD';
      if (offset < source.length + 1) {
        source = source.slice(0, offset) + wildcard + source.slice(offset);
      } else {
        source += wildcard;
      }
      offset += wildcard.length;
    }
    let localCtx = lspDriverHelper.createCtx(source, filePath, localCfg, this.globalContextPtr);
    const packageName = this.moduleInfos[filePath].packageName;
    const buildConfig = this.buildConfigs[packageName];
    PluginDriver.getInstance().getPluginContext().setCodingFilePath(filePath);
    PluginDriver.getInstance().getPluginContext().setProjectConfig(buildConfig);
    PluginDriver.getInstance().getPluginContext().setContextPtr(localCtx);
    lspDriverHelper.proceedToState(localCtx, Es2pandaContextState.ES2PANDA_STATE_PARSED);
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
    lspDriverHelper.proceedToState(localCtx, Es2pandaContextState.ES2PANDA_STATE_CHECKED);
    let ptr = global.es2panda._getCompletionAtPosition(localCtx, offset);
    PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    lspDriverHelper.destroyContext(localCtx);
    lspDriverHelper.destroyConfig(localCfg);
    return new LspCompletionInfo(ptr);
  }

  toLineColumnOffset(filename: String, offset: number): LspLineAndCharacter {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._toLineColumnOffset(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspLineAndCharacter(ptr);
  }

  getSafeDeleteInfo(filename: String, position: number): boolean {
    let result: boolean;
    const [cfg, ctx] = this.createContext(filename);
    try {
      result = global.es2panda._getSafeDeleteInfo(ctx, position);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return result;
  }

  findRenameLocations(filename: String, offset: number): LspRenameLocation[] {
    const [cfg, ctx] = this.createContext(filename);
    const needsCrossFileRename = global.es2panda._needsCrossFileRename(ctx, offset);
    if (!needsCrossFileRename) {
      let ptr: KPointer;
      try {
        ptr = global.es2panda._findRenameLocationsInCurrentFile(ctx, offset);
      } finally {
        this.destroyContext(cfg, ctx);
      }
      const result = new NativePtrDecoder().decode(ptr).map((elPeer: KPointer) => {
        return new LspRenameLocation(elPeer);
      });
      return Array.from(new Set(result));
    } else {
      let compileFiles = this.getMergedCompileFiles(filename);
      const fileContexts: KPointer[] = [];
      const fileConfigs: Config[] = [];
      for (let i = 0; i < compileFiles.length; i++) {
        const [compileFileCfg, compileFileCtx] = this.createContext(compileFiles[i]);
        fileContexts.push(compileFileCtx);
        fileConfigs.push(compileFileCfg);
      }
      const ptr = global.es2panda._findRenameLocations(
        fileContexts.length,
        passPointerArray(fileContexts),
        ctx,
        offset
      );
      const result: LspRenameLocation[] = new NativePtrDecoder().decode(ptr).map((elPeer: KPointer) => {
        return new LspRenameLocation(elPeer);
      });
      for (let i = 0; i < fileContexts.length; i++) {
        this.destroyContext(fileConfigs[i], fileContexts[i]);
      }
      this.destroyContext(cfg, ctx);
      return Array.from(new Set(result));
    }
  }

  getRenameInfo(filename: String, offset: number): LspRenameInfoType {
    let ptr: KPointer;
    let res: LspRenameInfoType;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getRenameInfo(ctx, offset, this.pandaLibPath);
      const success = global.es2panda._getRenameInfoIsSuccess(ptr);
      if (success) {
        res = new LspRenameInfoSuccess(global.es2panda._getRenameInfoSuccess(ptr));
      } else {
        res = new LspRenameInfoFailure(global.es2panda._getRenameInfoFailure(ptr));
      }
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return res;
  }

  getSpanOfEnclosingComment(filename: String, offset: number, onlyMultiLine: boolean): LspTextSpan {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getSpanOfEnclosingComment(ctx, offset, onlyMultiLine);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspTextSpan(ptr);
  }

  getCodeFixesAtPosition(filename: String, start: number, end: number, errorCodes: number[]): CodeFixActionInfo[] {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getCodeFixesAtPosition(ctx, start, end, new Int32Array(errorCodes), errorCodes.length);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    const codeFixActionInfoList = new CodeFixActionInfoList(ptr);
    const codeFixActionInfos: CodeFixActionInfo[] = [];
    codeFixActionInfos.push(...codeFixActionInfoList.codeFixActionInfos);
    return codeFixActionInfos;
  }

  provideInlayHints(filename: String, span: TextSpan): LspInlayHint[] {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      const nativeSpan = global.es2panda._createTextSpan(span.start, span.length);
      ptr = global.es2panda._getInlayHintList(ctx, nativeSpan);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    const inlayHintList = new LspInlayHintList(ptr);
    const inlayHints: LspInlayHint[] = [];
    inlayHints.push(...inlayHintList.inlayHints);
    return inlayHints;
  }

  getSignatureHelpItems(filename: String, offset: number): LspSignatureHelpItems {
    let ptr: KPointer;
    const [cfg, ctx] = this.createContext(filename);
    try {
      ptr = global.es2panda._getSignatureHelpItems(ctx, offset);
    } finally {
      this.destroyContext(cfg, ctx);
    }
    return new LspSignatureHelpItems(ptr);
  }

  // Use AST cache start
  private getFileDependencies(inputs: string[], output: string): void {
    let depInputContent = '';
    let outputFile: string = output;
    let depAnalyzerPath: string = path.join(this.pandaBinPath, 'dependency_analyzer');
    let depInputFile = path.join(this.cacheDir, 'depInput.txt');
    inputs.forEach((file) => {
      depInputContent += file + os.EOL;
    });
    fs.writeFileSync(depInputFile, depInputContent);
    ensurePathExists(outputFile);
    const result = child_process.spawnSync(
      depAnalyzerPath,
      [`@${depInputFile}`, `--output=${output}`, `--arktsconfig=${this.defaultArkTsConfig}`],
      {
        encoding: 'utf-8',
        windowsHide: true
      }
    );
    if (result.error || result.status !== 0) {
      console.error('getFileDependencies failed: ', result.stderr || result.error);
    }
  }

  // Collect circular dependencies, like: A→B→C→A
  private findStronglyConnectedComponents(graph: FileDepsInfo): Map<string, Set<string>> {
    const adjacencyList: Record<string, string[]> = {};
    const reverseAdjacencyList: Record<string, string[]> = {};
    const allNodes = new Set<string>();

    for (const node in graph.dependencies) {
      allNodes.add(node);
      graph.dependencies[node].forEach((dep) => allNodes.add(dep));
    }
    for (const node in graph.dependants) {
      allNodes.add(node);
      graph.dependants[node].forEach((dep) => allNodes.add(dep));
    }

    Array.from(allNodes).forEach((node) => {
      adjacencyList[node] = graph.dependencies[node] || [];
      reverseAdjacencyList[node] = graph.dependants[node] || [];
    });

    const visited = new Set<string>();
    const order: string[] = [];

    function dfs(node: string): void {
      visited.add(node);
      for (const neighbor of adjacencyList[node]) {
        if (!visited.has(neighbor)) {
          dfs(neighbor);
        }
      }
      order.push(node);
    }

    Array.from(allNodes).forEach((node) => {
      if (!visited.has(node)) {
        dfs(node);
      }
    });

    visited.clear();
    const components = new Map<string, Set<string>>();

    function reverseDfs(node: string, component: Set<string>): void {
      visited.add(node);
      component.add(node);
      for (const neighbor of reverseAdjacencyList[node]) {
        if (!visited.has(neighbor)) {
          reverseDfs(neighbor, component);
        }
      }
    }

    for (let i = order.length - 1; i >= 0; i--) {
      const node = order[i];
      if (!visited.has(node)) {
        const component = new Set<string>();
        reverseDfs(node, component);
        if (component.size > 1) {
          const sortedFiles = Array.from(component).sort();
          const hashKey = createHash(sortedFiles.join('|'));
          components.set(hashKey, component);
        }
      }
    }

    return components;
  }

  private getJobDependencies(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
    let depJobList: Set<string> = new Set<string>();
    fileDeps.forEach((file) => {
      if (!cycleFiles.has(file)) {
        depJobList.add('0' + file);
      } else {
        cycleFiles.get(file)?.forEach((f) => {
          depJobList.add(f);
        });
      }
    });

    return depJobList;
  }

  private getJobDependants(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
    let depJobList: Set<string> = new Set<string>();
    fileDeps.forEach((file) => {
      if (!file.endsWith(DECL_ETS_SUFFIX)) {
        depJobList.add('1' + file);
      }
      if (cycleFiles.has(file)) {
        cycleFiles.get(file)?.forEach((f) => {
          depJobList.add(f);
        });
      } else {
        depJobList.add('0' + file);
      }
    });

    return depJobList;
  }

  private collectCompileJobs(jobs: Record<string, Job>, isValid: boolean = false): void {
    let entryFileList: string[] = Object.keys(this.moduleInfos).filter((file) => {
      if (this.moduleInfos[file].language === LANGUAGE_VERSION.ARKTS_1_2) {
        return true;
      } else if (this.moduleInfos[file].language === LANGUAGE_VERSION.ARKTS_HYBRID) {
        const fileSource = this.getFileSource(file);
        return getFileLanguageVersion(fileSource) === LANGUAGE_VERSION.ARKTS_1_2;
      }
    });
    this.getFileDependencies(entryFileList, this.fileDependencies);
    const data = fs.readFileSync(this.fileDependencies, 'utf-8');
    let fileDepsInfo: FileDepsInfo = JSON.parse(data) as FileDepsInfo;

    Object.keys(fileDepsInfo.dependants).forEach((file) => {
      if (!(file in fileDepsInfo.dependencies)) {
        fileDepsInfo.dependencies[file] = [];
      }
    });

    let cycleGroups = this.findStronglyConnectedComponents(fileDepsInfo);

    let cycleFiles: Map<string, string[]> = new Map<string, string[]>();
    cycleGroups.forEach((value: Set<string>, key: string) => {
      value.forEach((file) => {
        cycleFiles.set(file, [key]);
      });
    });

    Object.entries(fileDepsInfo.dependencies).forEach(([key, value]) => {
      let dependencies = this.getJobDependencies(value, cycleFiles);
      if (cycleFiles.has(key)) {
        const externalProgramJobIds = cycleFiles.get(key)!;
        externalProgramJobIds.forEach((id) => {
          let fileList: string[] = Array.from(cycleGroups.get(id)!);
          this.createExternalProgramJob(id, fileList, jobs, dependencies, isValid, true);
        });
      } else {
        const id = '0' + key;
        let fileList: string[] = [key];
        this.createExternalProgramJob(id, fileList, jobs, dependencies, isValid);
      }
    });

    Object.entries(fileDepsInfo.dependants).forEach(([key, value]) => {
      const dependants = this.getJobDependants(value, cycleFiles);
      const jobIds = cycleFiles.has(key) ? cycleFiles.get(key)! : ['0' + key];

      jobIds.forEach((jobId) => {
        const currentDependants = new Set(dependants);
        jobs[jobId].dependants.forEach((dep) => currentDependants.add(dep));
        currentDependants.delete(jobId);
        jobs[jobId].dependants = Array.from(currentDependants);
      });
    });
  }

  private createExternalProgramJob(
    id: string,
    fileList: string[],
    jobs: Record<string, Job>,
    dependencies: Set<string>,
    isValid: boolean,
    isInCycle?: boolean
  ): void {
    if (dependencies.has(id)) {
      dependencies.delete(id);
    }
    if (jobs[id]) {
      const existingJob = jobs[id];
      const mergedFileList = [...new Set([...existingJob.fileList, ...fileList])];
      const mergedDependencies = [...new Set([...existingJob.dependencies, ...Array.from(dependencies)])];
      const mergedIsInCycle = existingJob.isInCycle || isInCycle;

      existingJob.fileList = mergedFileList;
      existingJob.dependencies = mergedDependencies;
      existingJob.isInCycle = mergedIsInCycle;
    } else {
      jobs[id] = {
        id,
        fileList,
        isDeclFile: true,
        isInCycle,
        dependencies: Array.from(dependencies),
        dependants: [],
        isValid
      };
    }
  }

  private addJobToQueues(job: Job, queues: Job[]): void {
    if (queues.some((j) => j.id === job.id)) {
      return;
    }
    queues.push(job);
  }

  private initCompileQueues(jobs: Record<string, Job>, queues: Job[], dependantJobs?: Record<string, Job>): void {
    Object.values(jobs).forEach((job) => {
      if (job.dependencies.length === 0) {
        if (dependantJobs && job.id in dependantJobs) {
          job.isValid = false;
          this.invalidateFileCache(job.fileList);
        }
        this.addJobToQueues(job, queues);
      }
    });
  }

  private checkAllTasksDone(queues: Job[], workerPool: WorkerInfo[]): boolean {
    if (queues.length === 0) {
      for (let i = 0; i < workerPool.length; i++) {
        if (!workerPool[i].isIdle) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  private initGlobalContext(jobs: Record<string, Job>): void {
    let files: string[] = [];
    Object.entries(jobs).forEach(([key, job]) => {
      for (let i = 0; i < job.fileList.length; i++) {
        files.push(job.fileList[i]);
      }
    });

    if (files.length === 0) {
      return;
    }

    let ets2pandaCmd: string[] = [
      '_',
      '--extension',
      'ets',
      '--arktsconfig',
      this.defaultArkTsConfig,
      Object.keys(this.moduleInfos)[0]
    ];

    this.globalLspDriverHelper = new LspDriverHelper();
    this.globalLspDriverHelper.memInitialize(this.pandaLibPath);
    this.globalConfig = this.globalLspDriverHelper.createCfg(ets2pandaCmd, files[0], this.pandaLibPath);
    this.globalContextPtr = this.globalLspDriverHelper.createGlobalContext(this.globalConfig.peer, files, files.length);
  }

  private updateQueues(
    jobs: Record<string, Job>,
    queues: Job[],
    jobId: string,
    dependantJobs?: Record<string, Job>
  ): void {
    const completedJob = jobs[jobId];
    completedJob.dependants.forEach((depJobId) => {
      const depJob = jobs[depJobId];
      if (!depJob) {
        return;
      }
      const depIndex = depJob.dependencies.indexOf(jobId);
      if (depIndex === -1) {
        return;
      }
      depJob.dependencies.splice(depIndex, 1);
      if (depJob.dependencies.length > 0) {
        return;
      }
      this.processCompletedDependencies(depJob, queues, dependantJobs);
    });
  }

  private processCompletedDependencies(depJob: Job, queues: Job[], dependantJobs?: Record<string, Job>): void {
    if (dependantJobs && depJob.id in dependantJobs) {
      depJob.isValid = false;
      this.invalidateFileCache(depJob.fileList);
    }
    this.addJobToQueues(depJob, queues);
  }

  private invalidateFileCache(fileList: string[]): void {
    fileList.forEach((file) => {
      global.es2pandaPublic._InvalidateFileCache(this.globalContextPtr!, file);
    });
  }

  private async invokeWorkers(
    jobs: Record<string, Job>,
    queues: Job[],
    processingJobs: Set<string>,
    workers: ThreadWorker[],
    numWorkers: number,
    dependantJobs?: Record<string, Job>
  ): Promise<void> {
    return new Promise<void>((resolve) => {
      const workerPool = this.createWorkerPool(numWorkers, workers);

      workerPool.forEach((workerInfo) => {
        this.setupWorkerListeners(workerInfo.worker, workerPool, jobs, queues, processingJobs, resolve, dependantJobs);
        this.assignTaskToIdleWorker(workerInfo, queues, processingJobs);
      });
    });
  }

  private createWorkerPool(numWorkers: number, workers: ThreadWorker[]): WorkerInfo[] {
    const workerPool: WorkerInfo[] = [];

    for (let i = 0; i < numWorkers; i++) {
      const worker = new ThreadWorker(path.resolve(__dirname, 'compile_thread_worker.js'), {
        workerData: { workerId: i }
      });
      workers.push(worker);
      workerPool.push({ worker, isIdle: true });
    }

    return workerPool;
  }

  private setupWorkerListeners(
    worker: ThreadWorker,
    workerPool: WorkerInfo[],
    jobs: Record<string, Job>,
    queues: Job[],
    processingJobs: Set<string>,
    resolve: () => void,
    dependantJobs?: Record<string, Job>
  ): void {
    worker.on('message', (msg) => {
      if (msg.type !== 'TASK_FINISH') {
        return;
      }

      this.handleTaskCompletion(msg.jobId, worker, workerPool, jobs, queues, processingJobs, dependantJobs);

      if (this.checkAllTasksDone(queues, workerPool)) {
        this.terminateWorkers(workerPool);
        resolve();
      }
    });
  }

  private handleTaskCompletion(
    jobId: string,
    worker: ThreadWorker,
    workerPool: WorkerInfo[],
    jobs: Record<string, Job>,
    queues: Job[],
    processingJobs: Set<string>,
    dependantJobs?: Record<string, Job>
  ): void {
    const workerInfo = workerPool.find((w) => w.worker === worker);
    if (workerInfo) {
      workerInfo.isIdle = true;
    }
    processingJobs.delete(jobId);
    this.updateQueues(jobs, queues, jobId, dependantJobs);
    workerPool.forEach((workerInfo) => {
      if (workerInfo.isIdle) {
        this.assignTaskToIdleWorker(workerInfo, queues, processingJobs);
      }
    });
  }

  private terminateWorkers(workerPool: WorkerInfo[]): void {
    workerPool.forEach(({ worker }) => {
      worker.postMessage({ type: 'EXIT' });
    });
  }

  private assignTaskToIdleWorker(workerInfo: WorkerInfo, queues: Job[], processingJobs: Set<string>): void {
    let job: Job | undefined;
    let jobInfo: JobInfo | undefined;

    if (queues.length > 0) {
      job = queues.shift()!;
      jobInfo = {
        id: job.id,
        filePath: job.fileList[0],
        arktsConfigFile: Object.prototype.hasOwnProperty.call(this.moduleInfos, job.fileList[0])
          ? this.moduleInfos[job.fileList[0]].arktsConfigFile
          : this.defaultArkTsConfig,
        globalContextPtr: this.globalContextPtr!,
        buildConfig: Object.values(this.buildConfigs)[0],
        isValid: job.isValid
      };
    }

    if (job) {
      processingJobs.add(job.id);
      workerInfo.worker.postMessage({ type: 'ASSIGN_TASK', jobInfo });
      workerInfo.isIdle = false;
    }
  }

  // AST caching is not enabled by default.
  // Call `initAstCache` before invoking the language service interface to enable AST cache
  public async initAstCache(numWorkers: number = 1): Promise<void> {
    const jobs: Record<string, Job> = {};
    const queues: Job[] = [];
    this.collectCompileJobs(jobs);
    this.initGlobalContext(jobs);
    this.initCompileQueues(jobs, queues);
    if (Object.keys(jobs).length === 0 && queues.length === 0) {
      return;
    }
    const processingJobs = new Set<string>();
    const workers: ThreadWorker[] = [];
    await this.invokeWorkers(jobs, queues, processingJobs, workers, numWorkers);
  }

  private compileExternalProgram(jobInfo: JobInfo): void {
    PluginDriver.getInstance().initPlugins(jobInfo.buildConfig);
    let ets2pandaCmd = ['-', '--extension', 'ets', '--arktsconfig', jobInfo.arktsConfigFile];
    let lspDriverHelper = new LspDriverHelper();
    let config = lspDriverHelper.createCfg(ets2pandaCmd, jobInfo.filePath);
    const source = fs.readFileSync(jobInfo.filePath, 'utf8').replace(/\r\n/g, '\n');
    let context = lspDriverHelper.createCtx(source, jobInfo.filePath, config, jobInfo.globalContextPtr, true);
    PluginDriver.getInstance().getPluginContext().setContextPtr(context);
    lspDriverHelper.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_PARSED);
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
    lspDriverHelper.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_LOWERED);
  }

  public addFileCache(filename: String): void {
    global.es2pandaPublic._AddFileCache(this.globalContextPtr!, filename);
    let jobInfo = {
      id: filename.valueOf(),
      filePath: filename.valueOf(),
      arktsConfigFile: Object.prototype.hasOwnProperty.call(this.moduleInfos, filename.valueOf())
        ? this.moduleInfos[filename.valueOf()].arktsConfigFile
        : this.defaultArkTsConfig,
      globalContextPtr: this.globalContextPtr!,
      buildConfig: Object.values(this.buildConfigs)[0],
      isValid: true
    };
    this.compileExternalProgram(jobInfo);
  }

  public removeFileCache(filename: String): void {
    global.es2pandaPublic._RemoveFileCache(this.globalContextPtr!, filename);
  }

  public async updateFileCache(filename: String, numWorkers: number = 1): Promise<void> {
    const queues: Job[] = [];
    const jobs: Record<string, Job> = {};
    this.collectCompileJobs(jobs, true);
    const dependantJobs = this.findJobDependants(jobs, filename.valueOf());
    this.initCompileQueues(jobs, queues, dependantJobs);
    const processingJobs = new Set<string>();
    const workers: ThreadWorker[] = [];
    await this.invokeWorkers(jobs, queues, processingJobs, workers, numWorkers, dependantJobs);
  }

  private findJobDependants(jobs: Record<string, Job>, filePath: string): Record<string, Job> {
    const targetJobs = this.findTargetJobs(jobs, filePath);
    const { visited, dependantJobs } = this.collectDependantJobs(jobs, targetJobs);

    return this.mergeJobs(targetJobs, dependantJobs);
  }

  private findTargetJobs(jobs: Record<string, Job>, filePath: string): Job[] {
    return Object.values(jobs).filter(
      (job) => job.fileList.includes(filePath) || (job.isInCycle && job.fileList.some((f) => f === filePath))
    );
  }

  private collectDependantJobs(
    jobs: Record<string, Job>,
    targetJobs: Job[]
  ): { visited: Set<string>; dependantJobs: Map<string, Job> } {
    const visited = new Set<string>();
    const dependantJobs = new Map<string, Job>();
    const queue: Job[] = [];

    targetJobs.forEach((job) => {
      if (!visited.has(job.id)) {
        visited.add(job.id);
        queue.push(job);
      }

      while (queue.length) {
        const current = queue.shift()!;
        this.processDependants(jobs, current, visited, queue, dependantJobs);
      }
    });

    return { visited, dependantJobs };
  }

  private processDependants(
    jobs: Record<string, Job>,
    current: Job,
    visited: Set<string>,
    queue: Job[],
    dependantJobs: Map<string, Job>
  ): void {
    current.dependants.forEach((dependantId) => {
      const dependantJob = jobs[dependantId];
      if (dependantJob && !visited.has(dependantId)) {
        visited.add(dependantId);
        queue.push(dependantJob);
        dependantJobs.set(dependantId, dependantJob);
      }
    });
  }

  private mergeJobs(targetJobs: Job[], dependantJobs: Map<string, Job>): Record<string, Job> {
    return [...targetJobs, ...dependantJobs.values()].reduce(
      (acc, job) => {
        acc[job.id] = job;
        return acc;
      },
      {} as Record<string, Job>
    );
  }

  public dispose(): void {
    this.globalLspDriverHelper!.destroyGlobalContext(this.globalContextPtr!);
    this.globalLspDriverHelper!.destroyConfig(this.globalConfig!);
    this.globalLspDriverHelper!.memFinalize();
  }
}

function createHash(str: string): string {
  const hash = crypto.createHash('sha256');
  hash.update(str);
  return hash.digest('hex');
}
// Use AST cache end
