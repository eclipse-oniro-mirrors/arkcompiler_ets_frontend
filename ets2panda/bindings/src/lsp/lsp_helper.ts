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
  LspLineAndCharacter,
  LspReferenceData,
  LspClassConstructorInfo,
  ApplicableRefactorItemInfo,
  LspApplicableRefactorInfo,
  LspRefactorEditInfo,
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
  LspNodeInfo,
  LspTokenTypeInfo,
  LspTokenNativeInfo,
  LspNode,
  ConstructorInfoFileTextChanges,
  FormatCodeSettingsOptions,
  LspFormattingTextChanges,
  TextChange
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
  NodeInfo
} from '../common/types';
import { PluginDriver, PluginHook } from '../common/ui_plugins_driver';
import { ModuleDescriptor, generateBuildConfigs } from './generateBuildConfig';
import { generateArkTsConfigs, generateModuleInfo } from './generateArkTSConfig';

import * as fs from 'fs';
import * as path from 'path';
import { KInt, KNativePointer, KPointer, KStringPtr } from '../common/InteropTypes';
import { passPointerArray } from '../common/private';
import { NativePtrDecoder } from '../common/Platform';
import { Worker as ThreadWorker } from 'worker_threads';
import { ensurePathExists, getFileLanguageVersion } from '../common/utils';
import * as child_process from 'child_process';
import { DECL_ETS_SUFFIX, DEFAULT_CACHE_DIR, LANGUAGE_VERSION, TS_SUFFIX } from '../common/preDefine';
import * as crypto from 'crypto';
import * as os from 'os';
import { changeDeclgenFileExtension } from '../common/utils';
import { logger } from './logger';
import { TextPositionUtils } from './utils';

const ets2pandaCmdPrefix = ['-', '--extension', 'ets', '--arktsconfig'];

function initBuildEnv(): void {
  const currentPath: string | undefined = process.env.PATH;
  let pandaLibPath: string = process.env.PANDA_LIB_PATH
    ? process.env.PANDA_LIB_PATH
    : path.resolve(__dirname, '../../../ets2panda/lib');
  process.env.PATH = `${currentPath}${path.delimiter}${pandaLibPath}`;
}

interface mainFileCache {
  fileContent: string;
  fileConfig: Config;
  fileContext: KNativePointer;
  fileHash: string;
}

export class Lsp {
  private pandaLibPath: string;
  private pandaBinPath: string;
  private getFileContent: (filePath: string) => string;
  private filesMap: Map<string, mainFileCache>; // Map<fileName, fileContent>
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

  constructor(
    pathConfig: PathConfig,
    getContentCallback?: (filePath: string) => string,
    modules?: ModuleDescriptor[],
    plugins?: string[]
  ) {
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
    this.filesMap = new Map<string, mainFileCache>();
    this.getFileContent = getContentCallback || ((path: string): string => fs.readFileSync(path, 'utf8'));
    this.buildConfigs = generateBuildConfigs(pathConfig, modules ? modules : [], plugins);
    this.moduleInfos = generateArkTsConfigs(this.buildConfigs);
    this.pathConfig = pathConfig;
    this.defaultArkTsConfig = Object.values(this.moduleInfos)[0].arktsConfigFile;
    this.defaultBuildConfig = Object.values(this.buildConfigs)[0];
    PluginDriver.getInstance().initPlugins(this.defaultBuildConfig);
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
    if (!fs.existsSync(fileName) || fs.statSync(fileName).isDirectory()) {
      return;
    }
    const hash = createHash(fileContent.newDoc);
    if (this.filesMap.get(fileName)?.fileHash === hash) {
      return;
    }
    if (this.filesMap.has(fileName)) {
      this.deleteFromFilesMap(fileName);
    }
    const [cfg, ctx] = this.createContext(fileName, true, fileContent.newDoc) ?? [];
    if (!cfg || !ctx) {
      return;
    }
    this.filesMap.set(fileName, { fileContent: fileContent.newDoc, fileConfig: cfg, fileContext: ctx, fileHash: hash });
  }

  deleteFromFilesMap(fileName: string): void {
    let fileCache = this.filesMap.get(fileName);
    if (fileCache) {
      this.destroyContext(fileCache.fileConfig, fileCache.fileContext);
      this.filesMap.delete(fileName);
    }
  }

  private getFileSource(filePath: string): string {
    const getSource =
      this.filesMap.get(filePath)?.fileContent || this.getFileContent(filePath) || fs.readFileSync(filePath, 'utf8');
    if (getSource === undefined) {
      logger.error('File content not found for path: ', filePath);
    }
    return getSource.replace(/\r\n/g, '\n');
  }

  private createContext(
    filename: String,
    processToCheck: boolean = true,
    fileSource?: string
  ): [Config, KNativePointer] | undefined {
    const filePath = path.resolve(filename.valueOf());
    const arktsconfig =
      process.env.ARKTSCONFIG ||
      (Object.prototype.hasOwnProperty.call(this.moduleInfos, filePath)
        ? this.moduleInfos[filePath].arktsConfigFile
        : this.defaultArkTsConfig);
    if (!arktsconfig) {
      logger.error('Missing arktsconfig for ', filePath);
    }

    const ets2pandaCmd = [...ets2pandaCmdPrefix, arktsconfig];
    const localCfg = this.lspDriverHelper.createCfg(ets2pandaCmd, filePath, this.pandaLibPath);
    const source = fileSource ? fileSource : this.getFileSource(filePath);

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

      this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, localCtx);
      PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

      if (processToCheck) {
        this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, localCtx);
      }
      return [localCfg, localCtx];
    } catch (error) {
      this.lspDriverHelper.destroyContext(localCtx);
      this.lspDriverHelper.destroyConfig(localCfg);
      logger.error('create context error for ', filePath);
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
      const [cfg, ctx] = this.createContext(filePath) ?? [];
      if (!cfg || !ctx) {
        return;
      }
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
        global.es2pandaPublic._GenerateTsDeclarationsFromContext(ctx, declEtsOutputPath, etsOutputPath, 1, 0, '', 1);
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

  getDefinitionAtPosition(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspDefinitionData | undefined {
    if (nodeInfos) {
      return this.getAtPositionByNodeInfos(filename, nodeInfos, 'definition') as LspDefinitionData;
    }
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getDefinitionAtPosition(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getDefinitionAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        ptr = global.es2panda._getDefinitionAtPosition(ctx, offset);
      } catch (error) {
        logger.error('failed to getDefinitionAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const result = new LspDefinitionData(ptr);
    const nodeInfoTemp: NodeInfo[] | undefined = this.getNodeInfos(filename, result.fileName, result.start);
    if (nodeInfoTemp !== undefined && nodeInfoTemp.length > 0) {
      result.nodeInfos = nodeInfoTemp;
    }
    return result;
  }

  private getMergedCompileFiles(filename: String): string[] {
    const moduleInfo = this.moduleInfos[path.resolve(filename.valueOf())];
    return moduleInfo ? [...moduleInfo.compileFiles, ...moduleInfo.depModuleCompileFiles] : [];
  }

  getSemanticDiagnostics(filename: String): LspDiagsNode | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getSemanticDiagnostics(fileCache.fileContext);
      } catch (error) {
        logger.error('failed to getSemanticDiagnostics by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getSemanticDiagnostics(ctx);
      } catch (error) {
        logger.error('failed to getSemanticDiagnostics', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspDiagsNode(ptr);
  }

  getCurrentTokenValue(filename: String, offset: number): string | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getCurrentTokenValue(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getCurrentTokenValue by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getCurrentTokenValue(ctx, offset);
      } catch (error) {
        logger.error('failed to getCurrentTokenValue', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return unpackString(ptr);
  }

  getImplementationAtPosition(filename: String, offset: number): LspDefinitionData | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getImplementationAtPosition(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getImplementationAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getImplementationAtPosition(ctx, offset);
      } catch (error) {
        logger.error('failed to getImplementationAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const result = new LspDefinitionData(ptr);
    return result;
  }

  getFileReferences(filename: String): LspReferenceData[] | undefined {
    let isPackageModule: boolean;
    let searchFileCache = this.filesMap.get(filename.valueOf());
    if (searchFileCache) {
      try {
        isPackageModule = global.es2panda._isPackageModule(searchFileCache.fileContext);
      } catch (error) {
        logger.error('failed to getFileReferences by fileCache', error);
        return;
      }
    } else {
      const [cfg, searchCtx] = this.createContext(filename) ?? [];
      if (!cfg || !searchCtx) { return; }
      try {
        isPackageModule = global.es2panda._isPackageModule(searchCtx);
      } catch (error) {
        logger.error('failed to getFileReferences', error);
        return;
      } finally {
        this.destroyContext(cfg, searchCtx);
      }
    }
    let result: LspReferenceData[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      let fileCache = this.filesMap.get(compileFiles[i].valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getFileReferences(path.resolve(filename.valueOf()), fileCache.fileContext, isPackageModule);
        } catch (error) {
          logger.error('failed to getFileReferences by fileCache', error);
          return;
        }
      } else {
        const [cfg, ctx] = this.createContext(compileFiles[i]) ?? [];
        if (!cfg || !ctx) { return; }
        try {
          ptr = global.es2panda._getFileReferences(path.resolve(filename.valueOf()), ctx, isPackageModule);
        } catch (error) {
          logger.error('failed to getFileReferences', error);
          return;
        } finally {
          this.destroyContext(cfg, ctx);
        }
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

  getReferencesAtPosition(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspReferenceData[] | undefined {
    if (nodeInfos) {
      return [this.getAtPositionByNodeInfos(filename, nodeInfos, 'reference') as LspReferenceData];
    }
    let declInfo: KPointer;
    let searchFileCache = this.filesMap.get(filename.valueOf());
    if (searchFileCache) {
      try {
        declInfo = global.es2panda._getDeclInfo(searchFileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getReferencesAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, searchCtx] = this.createContext(filename) ?? [];
      if (!cfg || !searchCtx) { return; }
      try {
        declInfo = global.es2panda._getDeclInfo(searchCtx, offset);
      } catch (error) {
        logger.error('failed to getReferencesAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, searchCtx);
      }
    }

    let result: LspReferenceData[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    const declFilesJson = this.moduleInfos[path.resolve(filename.valueOf())].declFilesPath;
    if (declFilesJson && declFilesJson.trim() !== '' && fs.existsSync(declFilesJson)) {
      this.addDynamicDeclFilePaths(declFilesJson, compileFiles);
    }
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      let fileCache = this.filesMap.get(compileFiles[i].valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getReferencesAtPosition(fileCache.fileContext, declInfo);
        } catch (error) {
          logger.error('failed to getReferencesAtPosition by fileCache', error);
          return;
        }
      } else {
        const [cfg, ctx] = this.createContext(compileFiles[i]) ?? [];
        if (!cfg || !ctx) { return; }
        try {
          ptr = global.es2panda._getReferencesAtPosition(ctx, declInfo);
        } catch (error) {
          logger.error('failed to getReferencesAtPosition', error);
          return;
        } finally {
          this.destroyContext(cfg, ctx);
        }
      }
      let refs = new LspReferences(ptr);
      if (refs.referenceInfos.length === 0) {
        continue;
      }
      refs.referenceInfos.forEach((ref) => {
        const nodeInfoTemp: NodeInfo[] | undefined = this.getNodeInfos(filename, ref.fileName, ref.start);
        if (nodeInfoTemp !== undefined && nodeInfoTemp.length > 0) {
          ref.nodeInfos = nodeInfoTemp;
        }
        result.push(ref);
      });
    }
    return Array.from(new Set(result));
  }

  private addDynamicDeclFilePaths(declFilesJson: string, compileFiles: string[]): void {
    try {
      const data = fs.readFileSync(declFilesJson, 'utf-8');
      const declFilesObj = JSON.parse(data);
      if (declFilesObj && declFilesObj.files) {
        Object.keys(declFilesObj.files).forEach((fileName) => {
          const fileItem = declFilesObj.files[fileName];
          if (fileItem && fileItem.declPath && compileFiles.indexOf(fileItem) < 0) {
            compileFiles.push(fileItem.declPath);
          }
        });
      }
    } catch (error) {
      console.error('Failed to parse declFilesJson:', error);
    }
  }

  private getNodeInfos(paramFileName: String, fileName: String, start: number): LspNodeInfo[] | undefined {
    let nodeInfos: LspNodeInfo[] = [];
    const moduleInfo = this.moduleInfos[path.resolve(paramFileName.valueOf())];
    if (!moduleInfo) {
      return;
    }
    const moduleName = moduleInfo.packageName;
    const declgenOutDir = this.buildConfigs[moduleName].declgenOutDir;
    if (
      (fileName.endsWith(DECL_ETS_SUFFIX) && fileName.startsWith(declgenOutDir)) ||
      (this.buildConfigs[moduleName].interopApiPath &&
        fileName.startsWith(this.buildConfigs[moduleName].interopApiPath!))
    ) {
      let ptr: KPointer;
      let fileCache = this.filesMap.get(fileName.valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getNodeInfosByDefinitionData(fileCache.fileContext, start);
        } catch (error) {
          logger.error('failed to getNodeInfos by fileCache', error);
          return;
        }
      } else {
        const [declFileCfg, declFileCtx] = this.createContext(fileName) ?? [];
        if (!declFileCfg || !declFileCtx) { return; }
        try {
          ptr = global.es2panda._getNodeInfosByDefinitionData(declFileCtx, start);
          nodeInfos = new NativePtrDecoder().decode(ptr).map((elPeer: KNativePointer) => {
            return new LspNodeInfo(elPeer);
          });
        } catch (error) {
          logger.error('failed to getNodeInfos', error);
          return;
        } finally {
          this.destroyContext(declFileCfg, declFileCtx);
        }
      }
    }
    return nodeInfos;
  }

  private getAtPositionByNodeInfos(
    declFilePath: String,
    nodeInfos: NodeInfo[],
    type: 'definition' | 'reference' | 'renameLocation'
  ): LspNode | undefined {
    let ptr: KPointer;
    let nodeInfoPtrs: KPointer[] = [];
    let sourceFilePath = this.declFileMap[declFilePath.valueOf()];
    if (sourceFilePath === undefined) {
      let unifiedPath = declFilePath.replace(/\\/g, '/');
      const targetSegment = 'build-tools/interop/declaration';
      if (unifiedPath.includes(targetSegment)) {
        unifiedPath = unifiedPath.replace(targetSegment, '');
        sourceFilePath = path.normalize(unifiedPath);
      }
    }
    let fileCache = this.filesMap.get(sourceFilePath.valueOf());
    if (fileCache) {
      try {
        nodeInfos.forEach((nodeInfo) => {
          nodeInfoPtrs.push(global.es2panda._CreateNodeInfoPtr(nodeInfo.name, nodeInfo.kind));
        });
        if (type === 'renameLocation') {
          ptr = global.es2panda._findRenameLocationsFromNode(fileCache.fileContext, passPointerArray(nodeInfoPtrs), nodeInfoPtrs.length);
        } else {
          ptr = global.es2panda._getDefinitionDataFromNode(fileCache.fileContext, passPointerArray(nodeInfoPtrs), nodeInfoPtrs.length);
        }
      } catch (error) {
        logger.error('failed to getAtPositionByNodeInfos by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(sourceFilePath) ?? [];
      if (!cfg || !ctx) { return };
      try {
        nodeInfos.forEach((nodeInfo) => {
          nodeInfoPtrs.push(global.es2panda._CreateNodeInfoPtr(nodeInfo.name, nodeInfo.kind));
        });
        if (type === 'renameLocation') {
          ptr = global.es2panda._findRenameLocationsFromNode(ctx, passPointerArray(nodeInfoPtrs), nodeInfoPtrs.length);
        } else {
          ptr = global.es2panda._getDefinitionDataFromNode(ctx, passPointerArray(nodeInfoPtrs), nodeInfoPtrs.length);
        }
      } catch (error) {
        logger.error('failed to getAtPositionByNodeInfos', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    switch (type) {
      case 'definition':
        return new LspDefinitionData(ptr, sourceFilePath);
      case 'reference':
        return new LspReferenceData(ptr, sourceFilePath);
      case 'renameLocation':
        return new LspRenameLocation(ptr, sourceFilePath);
      default:
        return new LspNodeInfo(ptr);
    }
  }

  getTypeHierarchies(filename: String, offset: number): LspTypeHierarchiesInfo | undefined {
    let ptr: KPointer;
    let ctxFile: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ctxFile = fileCache.fileContext;
        ptr = global.es2panda._getTypeHierarchies(ctxFile, ctxFile, offset);
      } catch (error) {
        logger.error('failed to getTypeHierarchies by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ctxFile = ctx;
        ptr = global.es2panda._getTypeHierarchies(ctxFile, ctxFile, offset);
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    let ref = new LspTypeHierarchiesInfo(ptr);
    if (ref.fileName === '') {
      return;
    }
    let result: LspTypeHierarchiesInfo[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let searchPtr: KPointer;
      let searchFileCache = this.filesMap.get(compileFiles[i].valueOf());
      if (searchFileCache) {
        try {
          searchPtr = global.es2panda._getTypeHierarchies(searchFileCache.fileContext, ctxFile, offset);
        } catch (error) {
          logger.error('failed to getTypeHierarchies by fileCache', error);
          return;
        }
      } else {
        const [cfg, searchCtx] = this.createContext(compileFiles[i]) ?? [];
        if (!cfg || !searchCtx) { return; }
        try {
          searchPtr = global.es2panda._getTypeHierarchies(searchCtx, ctxFile, offset);
        } catch (error) {
          logger.error('failed to getTypeHierarchies', error);
          return;
        } finally {
          this.destroyContext(cfg, searchCtx);
        }
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
      if (res) {
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

  getClassHierarchyInfo(filename: String, offset: number): LspClassHierarchy | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getClassHierarchyInfo(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getClassHierarchyInfo by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getClassHierarchyInfo(ctx, offset);
      } catch (error) {
        logger.error('failed to getClassHierarchyInfo', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspClassHierarchy(ptr);
  }

  getAliasScriptElementKind(filename: String, offset: number): LspCompletionEntryKind | undefined {
    let kind: KInt;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        kind = global.es2panda._getAliasScriptElementKind(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getAliasScriptElementKind by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        kind = global.es2panda._getAliasScriptElementKind(ctx, offset);
      } catch (error) {
        logger.error('failed to getAliasScriptElementKind', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return kind;
  }

  getClassHierarchies(filename: String, offset: number): LspClassHierarchies | undefined {
    let contextList = [];
    let nativeContextList: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      nativeContextList = global.es2panda._pushBackToNativeContextVector(fileCache.fileContext, fileCache.fileContext, 1)
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      contextList.push({ ctx: ctx, cfg: cfg });
      nativeContextList = global.es2panda._pushBackToNativeContextVector(ctx, ctx, 1);
    }
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let filePath = path.resolve(compileFiles[i]);
      if (path.resolve(filename.valueOf()) === filePath) {
        continue;
      }

      let searchFileCache = this.filesMap.get(filePath);
      if (searchFileCache) {
        global.es2panda._pushBackToNativeContextVector(searchFileCache.fileContext, nativeContextList, 0);
      } else {
        const [searchCfg, searchCtx] = this.createContext(filePath) ?? [];
        if (!searchCfg || !searchCtx) { return; }
        contextList.push({ ctx: searchCtx, cfg: searchCfg });
        global.es2panda._pushBackToNativeContextVector(searchCtx, nativeContextList, 0);
      }
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
  ): LspClassPropertyInfo | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getClassPropertyInfo(fileCache.fileContext, offset, shouldCollectInherited);
      } catch (error) {
        logger.error('failed to getClassPropertyInfo by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getClassPropertyInfo(ctx, offset, shouldCollectInherited);
      } catch (error) {
        logger.error('failed to getClassPropertyInfo', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspClassPropertyInfo(ptr);
  }

  getOrganizeImports(filename: String): LspFileTextChanges | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._organizeImports(fileCache.fileContext, filename);
        PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
      } catch (error) {
        logger.error('failed to getOrganizeImports by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._organizeImports(ctx, filename);
        PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
      } catch (error) {
        logger.error('failed to getOrganizeImports', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    return new LspFileTextChanges(ptr);
  }

  findSafeDeleteLocation(filename: String, offset: number): LspSafeDeleteLocationInfo[] | undefined {
    let declInfo: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        declInfo = global.es2panda._getDeclInfo(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to findSafeDeleteLocation by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        declInfo = global.es2panda._getDeclInfo(ctx, offset);
      } catch (error) {
        logger.error('failed to findSafeDeleteLocation', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    let result: LspSafeDeleteLocationInfo[] = [];
    let compileFiles = this.getMergedCompileFiles(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let ptr: KPointer;
      let searchFileCache = this.filesMap.get(compileFiles[i].valueOf());
      if (searchFileCache) {
        try {
          ptr = global.es2panda._findSafeDeleteLocation(searchFileCache.fileContext, declInfo);
        } catch (error) {
          logger.error('failed to findSafeDeleteLocation by fileCache', error);
          return;
        }
      } else {
        const [searchCfg, searchCtx] = this.createContext(compileFiles[i]) ?? [];
        if (!searchCfg || !searchCtx) { return; }
        try {
          ptr = global.es2panda._findSafeDeleteLocation(searchCtx, declInfo);
        } catch (error) {
          logger.error('failed to findSafeDeleteLocation', error);
          return;
        } finally {
          this.destroyContext(searchCfg, searchCtx);
        }
      }
      let refs = new LspSafeDeleteLocation(ptr);
      result.push(...refs.safeDeleteLocationInfos);
    }
    return Array.from(new Set(result));
  }

  getCompletionEntryDetails(filename: String, offset: number, entryName: String): CompletionEntryDetails | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getCompletionEntryDetails(entryName, filename, fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getCompletionEntryDetails by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getCompletionEntryDetails(entryName, filename, ctx, offset);
      } catch (error) {
        logger.error('failed to getCompletionEntryDetails', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    return new CompletionEntryDetails(ptr);
  }

  getApplicableRefactors(
    filename: String,
    kind: String,
    startPos: number,
    endPos: number
  ): ApplicableRefactorItemInfo[] | undefined {
    let ptr: KPointer;
    let result: ApplicableRefactorItemInfo[] = [];
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getApplicableRefactors(fileCache.fileContext, kind, startPos, endPos);
      } catch (error) {
        logger.error('failed to getApplicableRefactors by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getApplicableRefactors(ctx, kind, startPos, endPos);
      } catch (error) {
        logger.error('failed to getApplicableRefactors', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    let refs = new LspApplicableRefactorInfo(ptr);
    result.push(...refs.applicableRefactorInfo);
    return Array.from(new Set(result));
  }

  getEditsForRefactor(
    filename: string,
    refactorName: string,
    actionName: string,
    start: number,
    end: number,
    opts?: {
      userPrefsPtr?: KNativePointer | null;
      FormattingSettings?: KNativePointer | null;
    }
  ): LspRefactorEditInfo | undefined {
    let ptr: KPointer;
    let up = opts?.userPrefsPtr ?? BigInt(0);
    let fmt = opts?.FormattingSettings ?? BigInt(0);
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      ptr = global.es2panda._getEditsForRefactor(
        fileCache.fileContext,
        refactorName,
        actionName,
        start,
        end,
        up,
        fmt
      );
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      ptr = global.es2panda._getEditsForRefactor(
        ctx,
        refactorName,
        actionName,
        start,
        end,
        up,
        fmt
      );
      this.destroyContext(cfg, ctx);
    }
    return new LspRefactorEditInfo(ptr);
  }

  getClassConstructorInfo(filename: String, offset: number, properties: string[]): LspClassConstructorInfo | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getClassConstructorInfo(fileCache.fileContext, offset, passStringArray(properties));
      } catch (error) {
        logger.error('failed to getClassConstructorInfo by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getClassConstructorInfo(ctx, offset, passStringArray(properties));
      } catch (error) {
        logger.error('failed to getClassConstructorInfo', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspClassConstructorInfo(ptr);
  }

  getSyntacticDiagnostics(filename: String): LspDiagsNode | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getSyntacticDiagnostics(fileCache.fileContext);
      } catch (error) {
        logger.error('failed to getSyntacticDiagnostics by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename, false) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getSyntacticDiagnostics(ctx);
      } catch (error) {
        logger.error('failed to getSyntacticDiagnostics', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspDiagsNode(ptr);
  }

  getSuggestionDiagnostics(filename: String): LspDiagsNode | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getSuggestionDiagnostics(fileCache.fileContext);
      } catch (error) {
        logger.error('failed to getSuggestionDiagnostics by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getSuggestionDiagnostics(ctx);
      } catch (error) {
        logger.error('failed to getSuggestionDiagnostics', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspDiagsNode(ptr);
  }

  getQuickInfoAtPosition(filename: String, offset: number): LspQuickInfo | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getQuickInfoAtPosition(filename, fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getQuickInfoAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getQuickInfoAtPosition(filename, ctx, offset);
      } catch (error) {
        logger.error('failed to getQuickInfoAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const result = new LspQuickInfo(ptr);
    return result;
  }

  getDocumentHighlights(filename: String, offset: number): LspDocumentHighlightsReferences | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getDocumentHighlights(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getDocumentHighlights by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        ptr = global.es2panda._getDocumentHighlights(ctx, offset);
      } catch (error) {
        logger.error('failed to getDocumentHighlights', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const result = new LspDocumentHighlightsReferences(ptr);
    return result;
  }

  getCompletionAtPosition(filename: String, offset: number): LspCompletionInfo | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getCompletionAtPosition(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getCompletionAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getCompletionAtPosition(ctx, offset);
      } catch (error) {
        logger.error('failed to getCompletionAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspCompletionInfo(ptr);
  }

  toLineColumnOffset(filename: String, offset: number): LspLineAndCharacter | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._toLineColumnOffset(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to toLineColumnOffset by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        ptr = global.es2panda._toLineColumnOffset(ctx, offset);
      } catch (error) {
        logger.error('failed to toLineColumnOffset', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    return new LspLineAndCharacter(ptr);
  }

  getSafeDeleteInfo(filename: String, position: number): boolean | undefined {
    let result: boolean;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        result = global.es2panda._getSafeDeleteInfo(fileCache.fileContext, position);
      } catch (error) {
        logger.error('failed to getSafeDeleteInfo by fileCache', error);
        return;
      }
      return result;
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        result = global.es2panda._getSafeDeleteInfo(ctx, position);
      } catch (error) {
        logger.error('failed to getSafeDeleteInfo', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return result;
  }

  getTokenNative(filename: String, position: number): LspTokenNativeInfo {
    let result = new LspTokenNativeInfo();
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        let ptr = global.es2panda._getTokenTypes(fileCache.fileContext, position);
        let typeInfo = new LspTokenTypeInfo(ptr);
        if (typeof typeInfo.type === "string" && typeInfo.type.includes("native")) {
          return new LspTokenNativeInfo(typeInfo.name, true);
        }
        else {
          result = new LspTokenNativeInfo(typeInfo.name, false);
        }
      } catch (error) {
        console.error(error);
        throw error;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return result; }
      try {
        let ptr = global.es2panda._getTokenTypes(ctx, position);
        let typeInfo = new LspTokenTypeInfo(ptr);
        if (typeof typeInfo.type === "string" && typeInfo.type.includes("native")) {
          return new LspTokenNativeInfo(typeInfo.name, true);
        }
        else {
          result = new LspTokenNativeInfo(typeInfo.name, false);
        }
      } catch (error) {
        console.error(error);
        throw error;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return result;
  }

  findRenameLocations(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspRenameLocation[] | undefined {
    if (nodeInfos) {
      return [this.getAtPositionByNodeInfos(filename, nodeInfos, 'renameLocation') as LspRenameLocation];
    }

    let fileCache = this.filesMap.get(filename.valueOf());

    const processRenameLocations = (ctx: KNativePointer, isCached: boolean = false, cfg?: Config) => {
      const needsCrossFileRename = global.es2panda._needsCrossFileRename(ctx, offset);
      if (!needsCrossFileRename) {
        let ptr: KPointer;
        try {
          ptr = global.es2panda._findRenameLocationsInCurrentFile(ctx, offset);
        } catch (error) {
          logger.error('failed to findRenameLocations', error);
          if (!isCached && cfg) {
            this.destroyContext(cfg, ctx);
          }
          return undefined;
        }
        if (!isCached && cfg) {
          this.destroyContext(cfg, ctx);
        }
        const result = new NativePtrDecoder().decode(ptr).map((elPeer: KPointer) => {
          return new LspRenameLocation(elPeer);
        });
        return Array.from(new Set(result));
      } else {
        let compileFiles = this.getMergedCompileFiles(filename);
        const declFilesJson = this.moduleInfos[path.resolve(filename.valueOf())].declFilesPath;
        if (declFilesJson && declFilesJson.trim() !== '' && fs.existsSync(declFilesJson)) {
          this.addDynamicDeclFilePaths(declFilesJson, compileFiles);
        }

        const fileContexts: KPointer[] = [];
        const fileConfigs: Config[] = [];
        const tempContexts: { ctx: KNativePointer, cfg: Config }[] = [];

        for (let i = 0; i < compileFiles.length; i++) {
          let filePath = path.resolve(compileFiles[i]);
          if (filePath === path.resolve(filename.valueOf()) && isCached) {
            fileContexts.push(ctx);
            continue;
          }
          let searchFileCache = this.filesMap.get(filePath);
          if (searchFileCache) {
            fileContexts.push(searchFileCache.fileContext);
          } else {
            const [compileFileCfg, compileFileCtx] = this.createContext(compileFiles[i]) ?? [];
            if (!compileFileCfg || !compileFileCtx) {
              tempContexts.forEach(item => this.destroyContext(item.cfg, item.ctx));
              return undefined;
            }
            fileContexts.push(compileFileCtx);
            tempContexts.push({ ctx: compileFileCtx, cfg: compileFileCfg });
          }
        }

        let ptr: KPointer;
        try {
          ptr = global.es2panda._findRenameLocations(
            fileContexts.length,
            passPointerArray(fileContexts),
            ctx,
            offset
          );
        } catch (error) {
          logger.error('failed to findRenameLocations', error);
          tempContexts.forEach(item => this.destroyContext(item.cfg, item.ctx));
          if (!isCached && cfg) {
            this.destroyContext(cfg, ctx);
          }
          return undefined;
        }

        const result: LspRenameLocation[] = new NativePtrDecoder().decode(ptr).map((elPeer: KPointer) => {
          return new LspRenameLocation(elPeer);
        });

        result.forEach((ref) => {
          const nodeInfoTemp: NodeInfo[] | undefined = this.getNodeInfos(filename, ref.fileName, ref.start);
          if (nodeInfoTemp !== undefined && nodeInfoTemp.length > 0) {
            ref.nodeInfos = nodeInfoTemp;
          }
        });

        tempContexts.forEach(item => this.destroyContext(item.cfg, item.ctx));
        if (!isCached && cfg) {
          this.destroyContext(cfg, ctx);
        }

        return Array.from(new Set(result));
      }
    };

    if (fileCache) {
      return processRenameLocations(fileCache.fileContext, true);
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      return processRenameLocations(ctx, false, cfg);
    }
  }

  getRenameInfo(filename: String, offset: number): LspRenameInfoType | undefined {
    let ptr: KPointer;
    let res: LspRenameInfoType;

    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getRenameInfo(fileCache.fileContext, offset, this.pandaLibPath);
      } catch (error) {
        logger.error('failed to getRenameInfo by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getRenameInfo(ctx, offset, this.pandaLibPath);
      } catch (error) {
        logger.error('failed to getRenameInfo', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const success = global.es2panda._getRenameInfoIsSuccess(ptr);
    if (success) {
      res = new LspRenameInfoSuccess(global.es2panda._getRenameInfoSuccess(ptr));
    } else {
      res = new LspRenameInfoFailure(global.es2panda._getRenameInfoFailure(ptr));
    }
    return res;
  }

  getSpanOfEnclosingComment(filename: String, offset: number, onlyMultiLine: boolean): LspTextSpan | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getSpanOfEnclosingComment(fileCache.fileContext, offset, onlyMultiLine);
      } catch (error) {
        logger.error('failed to getSpanOfEnclosingComment by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getSpanOfEnclosingComment(ctx, offset, onlyMultiLine);
      } catch (error) {
        logger.error('failed to getSpanOfEnclosingComment', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    return new LspTextSpan(ptr);
  }

  getCodeFixesAtPosition(
    filename: String,
    start: number,
    end: number,
    errorCodes: number[]
  ): CodeFixActionInfo[] | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getCodeFixesAtPosition(fileCache.fileContext, start, end, new Int32Array(errorCodes), errorCodes.length);
      } catch (error) {
        logger.error('failed to getCodeFixesAtPosition by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getCodeFixesAtPosition(ctx, start, end, new Int32Array(errorCodes), errorCodes.length);
      } catch (error) {
        logger.error('failed to getCodeFixesAtPosition', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const codeFixActionInfoList = new CodeFixActionInfoList(ptr);
    const codeFixActionInfos: CodeFixActionInfo[] = [];
    codeFixActionInfos.push(...codeFixActionInfoList.codeFixActionInfos);
    return codeFixActionInfos;
  }

  provideInlayHints(filename: String, span: TextSpan): LspInlayHint[] | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        const nativeSpan = global.es2panda._createTextSpan(span.start, span.length);
        ptr = global.es2panda._getInlayHintList(fileCache.fileContext, nativeSpan);
      } catch (error) {
        logger.error('failed to provideInlayHints by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        const nativeSpan = global.es2panda._createTextSpan(span.start, span.length);
        ptr = global.es2panda._getInlayHintList(ctx, nativeSpan);
      } catch (error) {
        logger.error('failed to provideInlayHints', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const inlayHintList = new LspInlayHintList(ptr);
    const inlayHints: LspInlayHint[] = [];
    inlayHints.push(...inlayHintList.inlayHints);
    return inlayHints;
  }

  getSignatureHelpItems(filename: String, offset: number): LspSignatureHelpItems | undefined {
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getSignatureHelpItems(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getSignatureHelpItems by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) { return; }
      try {
        ptr = global.es2panda._getSignatureHelpItems(ctx, offset);
      } catch (error) {
        logger.error('failed to getSignatureHelpItems', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
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

  private dealWithJob(jobs: Record<string, Job>, queues: Job[]): void {
    let job: Job | undefined;
    let jobInfo: JobInfo | undefined;
    while (queues.length > 0) {
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
      this.compileExternalProgram(jobInfo);
      this.updateQueues(jobs, queues, job.id);
    }
  }

  // AST caching is not enabled by default.
  // Call `initAstCache` before invoking the language service interface to enable AST cache
  public initAstCache(): void {
    const jobs: Record<string, Job> = {};
    const queues: Job[] = [];
    this.collectCompileJobs(jobs);
    this.initGlobalContext(jobs);
    this.initCompileQueues(jobs, queues);
    if (Object.keys(jobs).length === 0 && queues.length === 0) {
      return;
    }
    this.dealWithJob(jobs, queues);
  }

  private compileExternalProgram(jobInfo: JobInfo): void {
    PluginDriver.getInstance().initPlugins(jobInfo.buildConfig);
    let ets2pandaCmd = ['-', '--extension', 'ets', '--arktsconfig', jobInfo.arktsConfigFile];
    let lspDriverHelper = new LspDriverHelper();
    let config = lspDriverHelper.createCfg(ets2pandaCmd, jobInfo.filePath);
    if (!fs.existsSync(jobInfo.filePath) || fs.statSync(jobInfo.filePath).isDirectory()) {
      logger.error('File content not found for path: ', jobInfo.filePath);
      return;
    }
    const source = fs.readFileSync(jobInfo.filePath, 'utf8').replace(/\r\n/g, '\n');
    let context = lspDriverHelper.createCtx(source, jobInfo.filePath, config, jobInfo.globalContextPtr, true, true);
    PluginDriver.getInstance().getPluginContext().setCodingFilePath(jobInfo.filePath);
    PluginDriver.getInstance().getPluginContext().setProjectConfig(config);
    PluginDriver.getInstance().getPluginContext().setContextPtr(context);
    lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, context);
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
    lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, context);
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

  public updateFileCache(filename: String) {
    const queues: Job[] = [];
    const jobs: Record<string, Job> = {};
    this.collectCompileJobs(jobs, true);
    const dependantJobs = this.findJobDependants(jobs, filename.valueOf());
    this.initCompileQueues(jobs, queues, dependantJobs);
    this.dealWithJob(jobs, queues);
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

  private boolToInt(value: boolean): number {
    return value ? 1 : 0;
  }

  private createFormatCodeSettings(options?: FormatCodeSettingsOptions): KNativePointer {
    const settingsPtr = global.es2panda._createFormatCodeSettings();

    if (options) {
      const booleanSettings: Array<[keyof FormatCodeSettingsOptions,
        (ptr: KNativePointer, val: number) => void]> = [
        ['insertSpaceAfterCommaDelimiter',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterCommaDelimiter],
        ['insertSpaceAfterSemicolonInForStatements',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterSemicolonInForStatements],
        ['insertSpaceBeforeAndAfterBinaryOperators',
          global.es2panda._setFormatCodeSettingsInsertSpaceBeforeAndAfterBinaryOperators],
        ['insertSpaceAfterConstructor',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterConstructor],
        ['insertSpaceAfterKeywordsInControlFlowStatements',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterKeywordsInControlFlowStatements],
        ['insertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterOpeningAndBeforeClosingNonemptyParenthesis],
        ['insertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterOpeningAndBeforeClosingNonemptyBrackets],
        ['insertSpaceAfterOpeningAndBeforeClosingNonemptyBraces',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterOpeningAndBeforeClosingNonemptyBraces],
        ['insertSpaceAfterOpeningAndBeforeClosingEmptyBraces',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterOpeningAndBeforeClosingEmptyBraces],
        ['insertSpaceAfterTypeAssertion',
          global.es2panda._setFormatCodeSettingsInsertSpaceAfterTypeAssertion],
        ['insertSpaceBeforeFunctionParenthesis',
          global.es2panda._setFormatCodeSettingsInsertSpaceBeforeFunctionParenthesis],
        ['placeOpenBraceOnNewLineForFunctions',
          global.es2panda._setFormatCodeSettingsPlaceOpenBraceOnNewLineForFunctions],
        ['placeOpenBraceOnNewLineForControlBlocks',
          global.es2panda._setFormatCodeSettingsPlaceOpenBraceOnNewLineForControlBlocks],
        ['insertSpaceBeforeTypeAnnotation',
          global.es2panda._setFormatCodeSettingsInsertSpaceBeforeTypeAnnotation],
        ['convertTabsToSpaces',
          global.es2panda._setFormatCodeSettingsConvertTabsToSpaces],
        ['trimTrailingWhitespace',
          global.es2panda._setFormatCodeSettingsTrimTrailingWhitespace],
        ['indentMultiLineObjectLiteralBeginningOnBlankLine',
          global.es2panda._setFormatCodeSettingsIndentMultiLineObjectLiteralBeginningOnBlankLine]
      ];

      for (const [key, setter] of booleanSettings) {
        const value = options[key];
        if (value !== undefined) {
          setter.call(global.es2panda, settingsPtr, this.boolToInt(value as boolean));
        }
      }

      const numberSettings: Array<[keyof FormatCodeSettingsOptions,
        (ptr: KNativePointer, val: number) => void]> = [
        ['indentSize', global.es2panda._setFormatCodeSettingsIndentSize],
        ['tabSize', global.es2panda._setFormatCodeSettingsTabSize],
        ['baseIndentSize', global.es2panda._setFormatCodeSettingsBaseIndentSize],
        ['indentStyle', global.es2panda._setFormatCodeSettingsIndentStyle],
        ['semicolons', global.es2panda._setFormatCodeSettingsSemicolons]
      ];

      for (const [key, setter] of numberSettings) {
        const value = options[key];
        if (value !== undefined) {
          setter.call(global.es2panda, settingsPtr, value as number);
        }
      }

      if (options.newLineCharacter !== undefined) {
        global.es2panda._setFormatCodeSettingsNewLineCharacter(settingsPtr, options.newLineCharacter);
      }
    }

    return settingsPtr;
  }

  getFormattingEditsForDocument(
    filename: String,
    options?: FormatCodeSettingsOptions
  ): TextChange[] | undefined {
    let ptr: KNativePointer;
    const settingsPtr = this.createFormatCodeSettings(options);

    try {
      let fileCache = this.filesMap.get(filename.valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getFormattingEditsForDocument(fileCache.fileContext, settingsPtr);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsForDocument by fileCache', error);
          return;
        }
      } else {
        const [cfg, ctx] = this.createContext(filename) ?? [];
        if (!cfg || !ctx) {
          return;
        }
        try {
          ptr = global.es2panda._getFormattingEditsForDocument(ctx, settingsPtr);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsForDocument', error);
          return;
        } finally {
          this.destroyContext(cfg, ctx);
        }
      }

      const result = new LspFormattingTextChanges(ptr);
      const changes = result.textChanges;
      result.dispose();
      return changes;
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
    }
  }

  getFormattingEditsForRange(
    filename: String,
    start: number,
    length: number,
    options?: FormatCodeSettingsOptions
  ): TextChange[] | undefined {
    let ptr: KNativePointer;
    const settingsPtr = this.createFormatCodeSettings(options);

    try {
      let fileCache = this.filesMap.get(filename.valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getFormattingEditsForRange(fileCache.fileContext, settingsPtr, start, length);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsForRange by fileCache', error);
          return;
        }
      } else {
        const [cfg, ctx] = this.createContext(filename) ?? [];
        if (!cfg || !ctx) {
          return;
        }
        try {
          ptr = global.es2panda._getFormattingEditsForRange(ctx, settingsPtr, start, length);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsForRange', error);
          return;
        } finally {
          this.destroyContext(cfg, ctx);
        }
      }

      const result = new LspFormattingTextChanges(ptr);
      const changes = result.textChanges;
      result.dispose();
      return changes;
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
    }
  }

  getFormattingEditsAfterKeystroke(
    filename: String,
    position: number,
    key: string,
    options?: FormatCodeSettingsOptions
  ): TextChange[] | undefined {
    if (!key || key.length === 0) {
      return;
    }
    let ptr: KNativePointer;
    const settingsPtr = this.createFormatCodeSettings(options);
    const keyCode = key.charCodeAt(0);

    try {
      let fileCache = this.filesMap.get(filename.valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getFormattingEditsAfterKeystroke(fileCache.fileContext, settingsPtr, position, 0, keyCode);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsAfterKeystroke by fileCache', error);
          return;
        }
      } else {
        const [cfg, ctx] = this.createContext(filename) ?? [];
        if (!cfg || !ctx) {
          return;
        }
        try {
          ptr = global.es2panda._getFormattingEditsAfterKeystroke(ctx, settingsPtr, position, 0, keyCode);
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        } catch (error) {
          logger.error('failed to getFormattingEditsAfterKeystroke', error);
          return;
        } finally {
          this.destroyContext(cfg, ctx);
        }
      }

      const result = new LspFormattingTextChanges(ptr);
      const changes = result.textChanges;
      result.dispose();
      return changes;
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
    }
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
