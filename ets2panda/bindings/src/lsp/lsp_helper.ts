/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
import { PerfMetricScope } from '../common/perf_helper';
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
import { passString, passStringArray, unpackString } from '../common/private';
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
import { KInt, KNativePointer, KPointer, KStringPtr } from '../common/InteropTypes';
import { passPointerArray } from '../common/private';
import { NativePtrDecoder } from '../common/Platform';
import { Worker as ThreadWorker } from 'worker_threads';
import { ensurePathExists, getFileLanguageVersion } from '../common/utils';
import * as child_process from 'child_process';
import { DECL_ETS_SUFFIX, DEFAULT_CACHE_DIR, LANGUAGE_VERSION, TS_SUFFIX } from '../common/preDefine';
import * as crypto from 'crypto';
import * as os from 'os';
import * as JSON5 from 'json5';
import { changeDeclgenFileExtension } from '../common/utils';
import { logger } from './logger';
import { TextPositionUtils, formEts2pandaCmd } from './utils';
import { ArkTSConfigGenerator } from '../common/arkTSConfigGenerator'

const ets2pandaCmdPrefix = ['-', '--extension', 'ets', '--arktsconfig'];
const LSP_OPTIONS = ['--unused-vars-analyzer'];

// Keep in sync with the C++ side: GetNodeInfoHandlers (node_matchers.cpp) decides which
// node kinds the extractor emits, and IsNodeInfoSymbolKind (api.cpp) gates them; a kind
// missing there never reaches this conversion table.
const NODE_INFO_KIND_CONVERSIONS: ReadonlyMap<AstNodeType, AstNodeType> = new Map([
  [AstNodeType.ETS_MODULE, AstNodeType.CLASS_DECLARATION],
  [AstNodeType.TS_MODULE_DECLARATION, AstNodeType.CLASS_DECLARATION],
  [AstNodeType.TS_ENUM_DECLARATION, AstNodeType.CLASS_DECLARATION],
  [AstNodeType.TS_ENUM_MEMBER, AstNodeType.CLASS_PROPERTY],
  [AstNodeType.FUNCTION_DECLARATION, AstNodeType.METHOD_DEFINITION]
]);

// Kinds whose node info chain elements can never be matched back by the node matchers
// (GetNodeMatchers in node_matchers.cpp); keep both lists aligned when adding kinds.
const UNMATCHABLE_NODE_KINDS: ReadonlySet<AstNodeType> = new Set([
  AstNodeType.EMPTY_STATEMENT,
  AstNodeType.EXPRESSION_STATEMENT,
  AstNodeType.RETURN_STATEMENT,
  AstNodeType.ETS_FUNCTION_TYPE,
  AstNodeType.TS_FUNCTION_TYPE,
  AstNodeType.FUNCTION_EXPRESSION
]);

function initBuildEnv(): void {
  const currentPath: string | undefined = process.env.PATH;
  let pandaLibPath: string = process.env.PANDA_LIB_PATH
    ? process.env.PANDA_LIB_PATH
    : path.resolve(__dirname, '../../../ets2panda/lib');
  process.env.PATH = `${currentPath}${path.delimiter}${pandaLibPath}`;
}

interface GlobalWithOptionalGc {
  gc?: () => void;
}

interface TimeoutWithUnref {
  unref?(): void;
}

interface mainFileCache {
  fileContent: string;
  fileConfig: Config;
  fileContext: KNativePointer;
  fileHash: string;
  arktsConfigPath: string;
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
  private pendingInvalidationRoot?: string;
  private compiledFileHashes: Map<string, string> = new Map<string, string>();
  private diagnosticFiles: Set<string> = new Set<string>();
  private pendingInitialDiagnosticFiles: Set<string> = new Set<string>();
  private inMemoryFileContents: Map<string, string> = new Map<string, string>();
  private buildConfigs: Record<string, BuildConfig>; // Map<moduleName, build_config.json>
  private moduleInfos: Record<string, ModuleInfo>; // Map<fileName, ModuleInfo>
  private reverseModuleDeps: Map<string, Set<string>>;
  private reverseClosureCache: Map<string, Set<string>> = new Map();
  private pathConfig: PathConfig;
  private lspDriverHelper = new LspDriverHelper();
  private declFileMap: Record<string, string> = {}; // Map<declFilePath, sourceFilePath>
  private indexedDynamicDecls: Set<string> = new Set(); // dynamic SDK decl files already indexed
  // Caches for the dynamic SDK decl indexing: the reference queries run on every cursor
  // idle (reference highlight), so the file scan and the arktsconfig parse must not be
  // repeated per query - a multi-megabyte arktsconfig parse per query would grow the
  // runtime heap high-water mark permanently.
  private dynamicImportScanCache: Map<string, { hash: string; imports: string[] }> = new Map();
  private dynamicDeclDependenciesCache: Map<string, Record<string, { path?: string }>> = new Map();
  private enablePerfMetric: boolean = false;
  private perfMetricLogFilePath: string = '';
  private kitFiles: string[] = [];
  private disposed: boolean = false;
  private idleRecycleTimer: ReturnType<typeof setTimeout> | undefined = undefined;
  private lastQueryTimestamp: number = 0;
  private consecutiveEditsCount: number = 0;
  private baselineRssMB: number = 0;
  private idleRecyclePostponeCount: number = 0;
  private lastRecycleTimestamp: number = 0;

  public static idleRecycleDelayMs: number = 5000;
  public static highMemoryRecycleDelayMs: number = 3000;
  public static queryActivityQuietWindowMs: number = 2000;
  public static consecutiveEditsThreshold: number = 25;
  public static highMemoryGrowthThresholdMB: number = 768;
  public static memoryGrowthThresholdMB: number = 1024;
  public static maxProcessRssLimitMB: number = 4096;
  public static maxIdleRecyclePostponeCount: number = 3;
  public static minRecycleIntervalMs: number = 30000;

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
    this.moduleInfos = generateArkTsConfigs(this.buildConfigs, this.kitFiles);
    this.reverseModuleDeps = this.buildReverseModuleDeps();
    this.pathConfig = pathConfig;
    this.defaultArkTsConfig = Object.values(this.moduleInfos)[0].arktsConfigFile;
    this.defaultBuildConfig = Object.values(this.buildConfigs)[0];
    PluginDriver.getInstance().initPlugins(this.defaultBuildConfig);
    this.lspDriverHelper.memInitialize(this.pandaLibPath);
    this.refreshBaselineRss('Lsp constructed');
    try {
      global.es2panda._initSymbolReferenceIndex();
    } catch (error) {
      logger.warn(`init symbol reference index failed: ${String(error)}`);
    }
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
    const prevBuildConfigs = this.buildConfigs;
    const prevReverseModuleDeps = this.reverseModuleDeps;
    const nextBuildConfigs = generateBuildConfigs(this.pathConfig, modules);
    const nextModuleInfos = generateArkTsConfigs(nextBuildConfigs);
    const invalidationRoots = this.collectInvalidationRoots(prevBuildConfigs, nextBuildConfigs);

    this.buildConfigs = nextBuildConfigs;
    this.moduleInfos = nextModuleInfos;
    this.reverseModuleDeps = this.buildReverseModuleDeps();
    this.invalidateReverseClosureCache(invalidationRoots, prevReverseModuleDeps, this.reverseModuleDeps);
  }

  deleteFile(fileName: string): void {
    this.inMemoryFileContents.delete(fileName);
    if (this.filesMap.size === 1) {
      const [, activeFileCache] = Array.from(this.filesMap.entries())[0];
      this.invalidateDependantsByRoot(activeFileCache.fileContext, fileName);
      global.es2pandaPublic._DeleteProgramForFile(activeFileCache.fileContext, fileName);
      this.tryRemoveSymbolReferenceIndex(fileName);
      this.compiledFileHashes.delete(fileName);
      this.diagnosticFiles.delete(fileName);
      this.pendingInitialDiagnosticFiles.delete(fileName);
      if (this.pendingInvalidationRoot === fileName) {
        this.pendingInvalidationRoot = undefined;
      }
    }
  }

  modifyFilesMap(fileName: string, fileContent: TextDocumentChangeInfo): void {
    this.inMemoryFileContents.set(fileName, fileContent.newDoc);
    this.consecutiveEditsCount++;
    this.checkAndProactivelyRecycleBeforeEdit();
    this.prepareFileCache(fileName, fileContent, true);
    this.scheduleIdleContextRecycle();
  }

  private getCurrentRssMB(): number {
    return typeof process !== 'undefined' && typeof process.memoryUsage === 'function'
      ? process.memoryUsage().rss / (1024 * 1024)
      : 0;
  }

  // The baseline marks the expected steady-state resident memory: the memory guard
  // only reacts to growth beyond it. It is captured at warm-up points (construction,
  // initAstCache, after each recycle), never in between.
  private refreshBaselineRss(reason: string): number {
    const rssMB = this.getCurrentRssMB();
    if (rssMB > 0) {
      this.baselineRssMB = rssMB;
    }
    logger.info(`[LSP MemGuard] Baseline RSS recorded at ${this.baselineRssMB.toFixed(1)} MB (${reason})`);
    return rssMB;
  }

  private isRecycleCooldownElapsed(): boolean {
    return this.lastRecycleTimestamp === 0 || Date.now() - this.lastRecycleTimestamp >= Lsp.minRecycleIntervalMs;
  }

  private checkAndProactivelyRecycleBeforeEdit(): void {
    if (this.disposed || this.isTestEnv() || this.filesMap.size === 0) {
      return;
    }
    const currentRssMB = this.getCurrentRssMB();
    const growthMB = currentRssMB - this.baselineRssMB;
    const rssLimitBreached = Lsp.maxProcessRssLimitMB > 0 && currentRssMB >= Lsp.maxProcessRssLimitMB;
    const growthBreached = this.baselineRssMB > 0 && growthMB >= Lsp.memoryGrowthThresholdMB;
    if (!rssLimitBreached && !growthBreached) {
      return;
    }
    // A growth breach within the cooldown defers to the idle channel: at this growth
    // level getAdaptiveRecycleDelay() already shortens the idle delay, while the
    // synchronous destroy-and-rebuild here is exactly what stalled the edit. Only the
    // hard RSS limit keeps the synchronous emergency path.
    if (!rssLimitBreached && !this.isRecycleCooldownElapsed()) {
      logger.info(
        `[LSP MemGuard] Memory growth before edit deferred to idle recycle (consecutive edits: ${this.consecutiveEditsCount}, growth: ${growthMB.toFixed(1)} MB, RSS: ${currentRssMB.toFixed(1)} MB)`
      );
      return;
    }
    logger.info(
      `[LSP MemGuard] Proactive emergency recycle triggered before edit (consecutive edits: ${this.consecutiveEditsCount}, growth: ${growthMB.toFixed(1)} MB, RSS: ${currentRssMB.toFixed(1)} MB)`
    );
    this.proactiveResetEHeap();
  }

  private proactiveResetEHeap(): void {
    this.clearIdleRecycleTimer();
    this.lastRecycleTimestamp = Date.now();
    this.disposeCachedContexts();
    if (global.es2panda && typeof global.es2panda._resetEHeap === 'function') {
      global.es2panda._resetEHeap();
    }
    const runtimeGlobal = globalThis as unknown as GlobalWithOptionalGc;
    if (typeof runtimeGlobal.gc === 'function') {
      runtimeGlobal.gc();
    }
    this.consecutiveEditsCount = 0;
    this.refreshBaselineRss('proactive emergency recycle');
  }

  public clearIdleRecycleTimer(): void {
    if (this.idleRecycleTimer !== undefined) {
      clearTimeout(this.idleRecycleTimer);
      this.idleRecycleTimer = undefined;
    }
  }

  private isTestEnv(): boolean {
    if (typeof process === 'undefined' || !process.env) {
      return false;
    }
    if (process.env.FORCE_LSP_IDLE_RECYCLE === 'true') {
      return false;
    }
    return (
      process.env.NODE_ENV === 'test' ||
      process.env.JEST_WORKER_ID !== undefined ||
      process.env.DISABLE_LSP_IDLE_RECYCLE === 'true'
    );
  }

  private getAdaptiveRecycleDelay(): number {
    const currentRssMB =
      typeof process !== 'undefined' && typeof process.memoryUsage === 'function'
        ? process.memoryUsage().rss / (1024 * 1024)
        : 0;
    const growthMB = currentRssMB - this.baselineRssMB;
    if (this.baselineRssMB > 0 && growthMB >= Lsp.highMemoryGrowthThresholdMB) {
      return Lsp.highMemoryRecycleDelayMs;
    }
    return Lsp.idleRecycleDelayMs;
  }

  private scheduleIdleContextRecycle(delayMs?: number): void {
    if (this.disposed || this.isTestEnv()) {
      return;
    }
    this.clearIdleRecycleTimer();
    if (delayMs === undefined) {
      this.idleRecyclePostponeCount = 0;
    }
    const delay = delayMs !== undefined ? delayMs : this.getAdaptiveRecycleDelay();
    this.idleRecycleTimer = setTimeout(() => {
      this.recycleContextOnIdle();
    }, delay);
    const timerObj = this.idleRecycleTimer as unknown as TimeoutWithUnref | undefined;
    if (timerObj && typeof timerObj.unref === 'function') {
      timerObj.unref();
    }
  }

  public isIdleRecycleTimerActive(): boolean {
    return this.idleRecycleTimer !== undefined;
  }

  public getConsecutiveEditsCount(): number {
    return this.consecutiveEditsCount;
  }

  public getIdleRecyclePostponeCount(): number {
    return this.idleRecyclePostponeCount;
  }

  public setLastQueryTimestampForTest(timestamp: number): void {
    this.lastQueryTimestamp = timestamp;
  }

  private shouldPostponeIdleRecycle(): boolean {
    const timeSinceLastQuery = Date.now() - this.lastQueryTimestamp;
    if (
      this.lastQueryTimestamp > 0 &&
      timeSinceLastQuery < Lsp.queryActivityQuietWindowMs &&
      this.idleRecyclePostponeCount < Lsp.maxIdleRecyclePostponeCount
    ) {
      this.idleRecyclePostponeCount++;
      logger.info(
        `[LSP MemGuard] Active query detected (${timeSinceLastQuery}ms ago). Postponing Context recycle by ${Lsp.queryActivityQuietWindowMs}ms (postpone count: ${this.idleRecyclePostponeCount}/${Lsp.maxIdleRecyclePostponeCount})`
      );
      this.scheduleIdleContextRecycle(Lsp.queryActivityQuietWindowMs);
      return true;
    }
    this.idleRecyclePostponeCount = 0;
    return false;
  }

  private rebuildActiveContext(
    activeFilePath: string,
    fileSource: string,
    hash: string,
    beforeRssMB: string,
    destroyCost: number
  ): void {
    if (this.disposed) {
      return;
    }
    const recreateStart = Date.now();
    const newCache = this.createFileCache(activeFilePath, fileSource, hash, true);
    const createCost = Date.now() - recreateStart;

    const afterRssMB = this.refreshBaselineRss('idle context recycle completed').toFixed(1);

    if (newCache !== undefined) {
      logger.info(
        `[LSP MemGuard] Context recycled successfully! Destroy: ${destroyCost}ms, Recreate: ${createCost}ms. RSS: ${beforeRssMB} MB -> ${afterRssMB} MB`
      );
    } else {
      logger.error(`[LSP MemGuard] Context recreate returned undefined for ${activeFilePath}`);
    }
  }

  public recycleContextOnIdle(): void {
    this.clearIdleRecycleTimer();
    if (this.disposed || this.filesMap.size === 0 || this.shouldPostponeIdleRecycle()) {
      return;
    }
    // Back-to-back recycles (post-recycle warm-up counted as fresh growth) made the
    // guard oscillate: rearm the timer for the rest of the cooldown instead.
    if (!this.isRecycleCooldownElapsed()) {
      const remainingMs = Math.max(Lsp.minRecycleIntervalMs - (Date.now() - this.lastRecycleTimestamp), 1);
      this.scheduleIdleContextRecycle(remainingMs);
      return;
    }

    const firstEntry = Array.from(this.filesMap.entries())[0];
    if (!firstEntry || !firstEntry[1]) {
      return;
    }
    const [activeFilePath, activeFileCache] = firstEntry;
    const fileSource = activeFileCache.fileContent;
    const hash = activeFileCache.fileHash;

    const beforeRssMB = this.getCurrentRssMB().toFixed(1);
    const startTime = Date.now();

    logger.info(
      `[LSP MemGuard] Idle detected without onDidChange. Starting Context recycle for: ${activeFilePath} (Current RSS: ${beforeRssMB} MB, Consecutive Edits: ${this.consecutiveEditsCount})`
    );

    try {
      this.lastRecycleTimestamp = Date.now();
      this.disposeCachedContexts();
      const destroyCost = Date.now() - startTime;

      if (global.es2panda && typeof global.es2panda._resetEHeap === 'function') {
        global.es2panda._resetEHeap();
      }
      const runtimeGlobal = globalThis as unknown as GlobalWithOptionalGc;
      if (typeof runtimeGlobal.gc === 'function') {
        runtimeGlobal.gc();
      }
      this.consecutiveEditsCount = 0;
      this.rebuildActiveContext(activeFilePath, fileSource, hash, beforeRssMB, destroyCost);
    } catch (error) {
      logger.error(`[LSP MemGuard] Context recycle exception: ${String(error)}`);
    }
  }

  private normalizePathForComparison(fileName: string): string {
    const normalizedPath = path.normalize(path.resolve(fileName));
    return process.platform === 'win32' ? normalizedPath.toLowerCase() : normalizedPath;
  }

  private getModuleInfo(fileName: string): ModuleInfo | undefined {
    const filePath = path.resolve(fileName);
    const moduleInfo = this.moduleInfos[filePath];
    if (moduleInfo || process.platform !== 'win32') {
      return moduleInfo;
    }

    const normalizedFilePath = this.normalizePathForComparison(filePath);
    const moduleFilePath = Object.keys(this.moduleInfos).find(
      (candidate) => this.normalizePathForComparison(candidate) === normalizedFilePath
    );
    return moduleFilePath === undefined ? undefined : this.moduleInfos[moduleFilePath];
  }

  private getArkTsConfigPath(fileName: string, preferModuleArkTsConfig: boolean): string {
    const moduleArkTsConfig = this.getModuleInfo(fileName)?.arktsConfigFile;
    const arkTsConfig = preferModuleArkTsConfig
      ? moduleArkTsConfig || process.env.ARKTSCONFIG || this.defaultArkTsConfig
      : process.env.ARKTSCONFIG || moduleArkTsConfig || this.defaultArkTsConfig;
    if (!arkTsConfig) {
      logger.error('Missing arktsconfig for ', path.resolve(fileName));
    }
    return arkTsConfig;
  }

  private hasSameArkTsConfig(left: string, right: string): boolean {
    return this.normalizePathForComparison(left) === this.normalizePathForComparison(right);
  }

  private withTemporaryFileCacheForQuery<T>(
    fileName: string,
    query: (fileCache: mainFileCache) => T | undefined
  ): T | undefined {
    let fileSource: string;
    try {
      fileSource = this.getFileSource(fileName);
    } catch (error) {
      logger.error('failed to read file for query context', error);
      return undefined;
    }

    const [cfg, ctx] = this.createContextForCrossModule(fileName, true, fileSource) ?? [];
    if (!cfg || !ctx) {
      return undefined;
    }

    const fileCache: mainFileCache = {
      fileContent: fileSource,
      fileConfig: cfg,
      fileContext: ctx,
      fileHash: this.computeContentHash(fileSource),
      arktsConfigPath: this.getArkTsConfigPath(fileName, true)
    };
    this.tryBuildSymbolReferenceIndexWithExternal(fileName, ctx);
    try {
      return query(fileCache);
    } finally {
      this.destroyContext(cfg, ctx);
    }
  }

  private ensureFileCacheForQuery(fileName: string, forceRecompile: boolean = false): mainFileCache | undefined {
    const fileCache = this.filesMap.get(fileName);
    if (fileCache && !forceRecompile) {
      return fileCache;
    }

    try {
      return this.prepareFileCache(fileName, { newDoc: this.getFileSource(fileName) }, true, forceRecompile);
    } catch (error) {
      logger.error('failed to prepare file cache for query', error);
      return undefined;
    }
  }

  private restoreFileCacheForQuery(fileName: string, fileCache: mainFileCache): void {
    const ret = global.es2pandaPublic._IncrementalPrepareProgram(
      fileCache.fileContext,
      fileName,
      fileCache.fileContent,
      0
    );
    if (ret !== 0 && ret !== 1) {
      logger.error('restore file cache failed for ' + fileName + ', ret: ' + ret);
      return;
    }
    if (ret === 0) {
      this.proceedPreparedContext(fileName, fileCache.fileContext, true);
      this.tryBuildSymbolReferenceIndexWithExternal(fileName, fileCache.fileContext);
    }
    this.recordCompilationState(fileName, fileCache.fileHash, ret === 0);
    this.filesMap.clear();
    this.filesMap.set(fileName, fileCache);
  }

  private withFileCacheForQuery<T>(
    filename: String | string,
    query: (fileCache: mainFileCache) => T | undefined,
    forceRecompile: boolean = false
  ): T | undefined {
    this.lastQueryTimestamp = Date.now();
    const fileName = filename.valueOf();
    const activeFileEntry = Array.from(this.filesMap.entries())[0];
    const shouldRestoreActiveFile = activeFileEntry !== undefined && activeFileEntry[0] !== fileName;

    const targetModuleArkTsConfig = this.getModuleInfo(fileName)?.arktsConfigFile;
    if (
      activeFileEntry !== undefined &&
      targetModuleArkTsConfig !== undefined &&
      !this.hasSameArkTsConfig(activeFileEntry[1].arktsConfigPath, targetModuleArkTsConfig)
    ) {
      // A context keeps the ArkTSConfig it was created with; isolate cross-module queries from the active cache.
      return this.withTemporaryFileCacheForQuery(fileName, query);
    }

    try {
      const fileCache = this.ensureFileCacheForQuery(fileName, forceRecompile);
      if (!fileCache) {
        return undefined;
      }
      return query(fileCache);
    } finally {
      if (shouldRestoreActiveFile) {
        this.restoreFileCacheForQuery(activeFileEntry[0], activeFileEntry[1]);
      }
    }
  }

  private prepareFileCache(
    fileName: string,
    fileContent: TextDocumentChangeInfo,
    preferModuleArkTsConfig: boolean = false,
    forceRecompile: boolean = false
  ): mainFileCache | undefined {
    if (!fs.existsSync(fileName) || fs.statSync(fileName).isDirectory()) {
      return undefined;
    }

    const fileSource = fileContent.newDoc;
    const hash = this.computeContentHash(fileSource);
    const cachedFile = this.filesMap.get(fileName);
    const targetModuleArkTsConfig = this.getModuleInfo(fileName)?.arktsConfigFile;
    const targetArkTsConfig = this.getArkTsConfigPath(fileName, preferModuleArkTsConfig);
    if (
      cachedFile !== undefined &&
      cachedFile.fileHash === hash &&
      (targetModuleArkTsConfig === undefined ||
        this.hasSameArkTsConfig(cachedFile.arktsConfigPath, targetArkTsConfig)) &&
      !forceRecompile
    ) {
      return cachedFile;
    }

    if (this.filesMap.size === 1) {
      const [, activeFileCache] = Array.from(this.filesMap.entries())[0];
      const activeConfigMatches =
        targetModuleArkTsConfig === undefined ||
        this.hasSameArkTsConfig(activeFileCache.arktsConfigPath, targetArkTsConfig);
      if (!activeConfigMatches) {
        this.destroyContext(activeFileCache.fileConfig, activeFileCache.fileContext);
        this.filesMap.clear();
        return this.createFileCache(fileName, fileSource, hash, preferModuleArkTsConfig);
      }
      return this.prepareFileCacheFromActiveContext(fileName, fileSource, hash, forceRecompile);
    }
    return this.createFileCache(fileName, fileSource, hash, preferModuleArkTsConfig);
  }

  private prepareFileCacheFromActiveContext(
    fileName: string,
    fileSource: string,
    hash: string,
    forceRecompile: boolean = false
  ): mainFileCache | undefined {
    const [activeFilePath, activeFileCache] = Array.from(this.filesMap.entries())[0];
    if (activeFilePath !== fileName && this.shouldInvalidateDependantsOnSwitch(activeFilePath)) {
      this.invalidatePendingDependants(activeFileCache.fileContext);
    }

    const previousCompiledHash = this.compiledFileHashes.get(fileName);
    const diskHash = previousCompiledHash === undefined ? this.readDiskHash(fileName) : undefined;
    const isChanged = previousCompiledHash !== undefined
      ? previousCompiledHash !== hash
      : diskHash !== undefined ? diskHash !== hash : true;
    const shouldRecompile = forceRecompile || isChanged || this.diagnosticFiles.has(fileName);
    const ret = global.es2pandaPublic._IncrementalPrepareProgram(
      activeFileCache.fileContext,
      fileName,
      fileSource,
      shouldRecompile ? 1 : 0
    );
    if (ret !== 0 && ret !== 1) {
      logger.error('incremental update failed for ' + fileName + ', ret: ' + ret);
      return undefined;
    }
    if (ret === 0) {
      this.proceedPreparedContext(fileName, activeFileCache.fileContext, true);
      this.tryBuildSymbolReferenceIndexWithExternal(fileName, activeFileCache.fileContext);
      this.updateDiagnosticFilesForContext(fileName, activeFileCache.fileContext);
    }

    this.recordCompilationState(fileName, hash, ret === 0);
    const updatedFileCache = {
      fileContent: fileSource,
      fileConfig: activeFileCache.fileConfig,
      fileContext: activeFileCache.fileContext,
      fileHash: hash,
      arktsConfigPath: activeFileCache.arktsConfigPath
    };
    this.filesMap.clear();
    this.filesMap.set(fileName, updatedFileCache);
    return updatedFileCache;
  }

  private createFileCache(
    fileName: string,
    fileSource: string,
    hash: string,
    preferModuleArkTsConfig: boolean
  ): mainFileCache | undefined {
    // Query misses should use the target file's module config before any global ARKTSCONFIG.
    const [cfg, ctx] = preferModuleArkTsConfig
      ? this.createContextForCrossModule(fileName, true, fileSource) ?? []
      : this.createContext(fileName, true, fileSource) ?? [];
    if (!cfg || !ctx) {
      return undefined;
    }

    this.tryBuildSymbolReferenceIndexWithExternal(fileName, ctx);
    this.recordCompilationState(fileName, hash);
    const newFileCache = {
      fileContent: fileSource,
      fileConfig: cfg,
      fileContext: ctx,
      fileHash: hash,
      arktsConfigPath: this.getArkTsConfigPath(fileName, preferModuleArkTsConfig)
    };
    this.filesMap.set(fileName, newFileCache);
    return newFileCache;
  }

  deleteFromFilesMap(fileName: string): void {
    let fileCache = this.filesMap.get(fileName);
    if (fileCache) {
      this.destroyContext(fileCache.fileConfig, fileCache.fileContext);
      this.filesMap.delete(fileName);
      this.pendingInitialDiagnosticFiles.delete(fileName);
      if (this.filesMap.size === 0) {
        this.clearIdleRecycleTimer();
      }
    }
  }

  private tryBuildSymbolReferenceIndex(filePath: string, context: KNativePointer): void {
    try {
      const ret = global.es2panda._buildSymbolReferenceIndexForContext(context);
      if (!ret) {
        logger.warn(`build symbol reference index failed for ${filePath}`);
      }
    } catch (error) {
      logger.warn(`build symbol reference index exception for ${filePath}: ${String(error)}`);
    }
  }

  // The external build also indexes the imported declaration programs (including the
  // static / dynamic SDK declaration files), so references inside them are reported by
  // the index-based reference queries. Runs when a file program is (re)introduced into
  // a context - first cache entry, incremental re-introduction, cross-module temporary
  // context - so the declaration content enters the index once per introduction instead
  // of being rebuilt on every reference query.
  private tryBuildSymbolReferenceIndexWithExternal(filePath: string, context: KNativePointer): void {
    try {
      if (global.es2panda && typeof global.es2panda._buildSymbolReferenceIndexForContextWithExternal === 'function') {
        const ret = global.es2panda._buildSymbolReferenceIndexForContextWithExternal(context);
        if (!ret) {
          logger.warn(`build symbol reference index with external failed for ${filePath}`);
        }
      }
    } catch (error) {
      logger.warn(`build symbol reference index with external exception for ${filePath}: ${String(error)}`);
    }
  }

  private tryRemoveSymbolReferenceIndex(filePath: string): void {
    try {
      global.es2panda._removeSymbolReferenceIndexForFile(filePath);
    } catch (error) {
      logger.warn(`remove symbol reference index exception for ${filePath}: ${String(error)}`);
    }
  }

  private getFileSource(filePath: string): string {
    const getSource =
      this.filesMap.get(filePath)?.fileContent ||
      this.inMemoryFileContents.get(filePath) ||
      this.getFileContent(filePath) ||
      fs.readFileSync(filePath, 'utf8');
    if (getSource === undefined) {
      logger.error('File content not found for path: ', filePath);
    }
    return getSource.replace(/\r\n/g, '\n');
  }

  private readDiskHash(fileName: string): string | undefined {
    try {
      const diskSource = fs.readFileSync(fileName, 'utf8');
      return this.computeContentHash(diskSource);
    } catch (error) {
      logger.warn(`readDiskHash failed for ${fileName}: ${String(error)}`);
      return undefined;
    }
  }

  private computeContentHash(fileSource: string): string {
    return createHash(fileSource.replace(/\r\n/g, '\n'));
  }

  private shouldInvalidateDependantsOnSwitch(activeFilePath: string): boolean {
    return this.pendingInvalidationRoot === activeFilePath;
  }

  private recordCompilationState(
    fileName: string,
    hash: string,
    allowDiskBaseline: boolean = true
  ): void {
    const previousHash = this.compiledFileHashes.get(fileName);
    const baselineHash = previousHash ?? (allowDiskBaseline ? this.readDiskHash(fileName) : undefined);
    if (baselineHash !== undefined && baselineHash !== hash) {
      this.pendingInvalidationRoot = fileName;
    }

    this.compiledFileHashes.set(fileName, hash);
  }

  private invalidatePendingDependants(context: KNativePointer): void {
    if (!this.pendingInvalidationRoot) {
      return;
    }

    this.invalidateDependantsByRoot(context, this.pendingInvalidationRoot);
    this.pendingInvalidationRoot = undefined;
  }

  private invalidateDependantsByRoot(
    context: KNativePointer,
    rootFilePath: string
  ): void {
    const ret = global.es2pandaPublic._DeleteDependantProgramsForFiles(context, rootFilePath);
    if (ret !== 0) {
      logger.warn(`batch delete dependant programs failed, ret=${ret}`);
    }
  }

  private markDiagnosticFile(fileName: string, diagnostics: LspDiagsNode): void {
    if (fileName && diagnostics.diagnostics.length > 0) {
      this.diagnosticFiles.add(fileName);
    }
  }

  private updateDiagnosticFilesForContext(fileName: string, context: KNativePointer): void {
    if (!fileName) {
      return;
    }

    this.diagnosticFiles.delete(fileName);
    const semanticDiagnostics = new LspDiagsNode(global.es2panda._getSemanticDiagnostics(context));
    const syntacticDiagnostics = new LspDiagsNode(global.es2panda._getSyntacticDiagnostics(context));
    if (semanticDiagnostics.diagnostics.length > 0 || syntacticDiagnostics.diagnostics.length > 0) {
      this.diagnosticFiles.add(fileName);
    }
  }

  private proceedPreparedContext(filePath: string, context: KNativePointer, processToCheck: boolean = true): void {
    const packageName = Object.prototype.hasOwnProperty.call(this.moduleInfos, filePath)
      ? this.moduleInfos[filePath].packageName
      : undefined;
    const buildConfig = packageName ? this.buildConfigs[packageName] : this.defaultBuildConfig;
    const pluginDriver = PluginDriver.getInstance();
    pluginDriver.initPlugins(buildConfig);
    const pluginContext = pluginDriver.getPluginContext();
    pluginContext.setCodingFilePath(filePath);
    pluginContext.setProjectConfig(buildConfig);
    pluginContext.setContextPtr(context);

    this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, context);
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

    if (processToCheck) {
      this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, context);
      PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
    }
  }

  private createContext(
    filename: String,
    processToCheck: boolean = true,
    fileSource?: string
  ): [Config, KNativePointer] | undefined {
    return this.createContextByArkTsConfigPriority(filename, processToCheck, fileSource, false);
  }

  private createContextForCrossModule(
    filename: String,
    processToCheck: boolean = true,
    fileSource?: string
  ): [Config, KNativePointer] | undefined {
    return this.createContextByArkTsConfigPriority(
      filename,
      processToCheck,
      fileSource,
      Object.keys(this.buildConfigs).length > 1
    );
  }

  private createContextByArkTsConfigPriority(
    filename: String,
    processToCheck: boolean,
    fileSource: string | undefined,
    preferModuleArkTsConfig: boolean
  ): [Config, KNativePointer] | undefined {
    const filePath = path.resolve(filename.valueOf());
    const arktsconfig = this.getArkTsConfigPath(filePath, preferModuleArkTsConfig);

    const ets2pandaCmd = [...ets2pandaCmdPrefix, arktsconfig, ...LSP_OPTIONS];
    const localCfg = this.lspDriverHelper.createCfg(ets2pandaCmd, filePath, this.pandaLibPath);
    const source = fileSource ? fileSource : this.getFileSource(filePath);

    const localCtx = this.lspDriverHelper.createCtx(source, filePath, localCfg, this.globalContextPtr, false, true);
    try {
      this.proceedPreparedContext(filePath, localCtx, processToCheck);
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

  private generateDeclFileByContext(ctx: KNativePointer, filePath: string): void {
    const moduleInfo = this.moduleInfos[filePath];
    const modulePath: string = path.relative(moduleInfo.moduleRootPath, filePath);
    const declEtsOutputPath: string = changeDeclgenFileExtension(
      path.join(moduleInfo.declgenV1OutPath!, modulePath),
      DECL_ETS_SUFFIX
    );
    this.declFileMap[declEtsOutputPath] = filePath;
    ensurePathExists(declEtsOutputPath);
    const declgen = global.es2pandaPublic._CreateTsDeclgen(ctx, 1,
      passStringArray([filePath]),
      passStringArray([declEtsOutputPath]),
      passStringArray(['']), 1, 0, passString(''), 1
    );
    try {
      global.es2pandaPublic._GenerateTsDeclarationsAfterParsed(declgen);
      this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, ctx);
      PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
      global.es2pandaPublic._GenerateTsDeclarationsAfterCheck(declgen);
      global.es2pandaPublic._WriteTsDeclarations(declgen);
    }
    finally {
      global.es2pandaPublic._DestroyTsDeclgen(declgen);
    }
  }

  private generateDeclFile(filePath: string): void {
    const fileSource = this.getFileSource(filePath);
    if (getFileLanguageVersion(fileSource) === LANGUAGE_VERSION.ARKTS_1_2) {
      const fileCache = this.filesMap.get(filePath);
      if (fileCache) {
        this.generateDeclFileByContext(fileCache.fileContext, filePath);
      } else {
        const [cfg, ctx] = this.createContext(filePath, false) ?? [];
        if (!cfg || !ctx) {
          return;
        }
        try {
          this.generateDeclFileByContext(ctx, filePath);
        } finally {
          this.destroyContext(cfg, ctx);
        }
      }
    }
  }

  private initDeclFile(ctx: KNativePointer): void {
    for (const filePath of Object.keys(this.moduleInfos)) {
      this.generateDeclFileByContext(ctx, filePath);
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
    const utf16Offset = TextPositionUtils.lineAndColumnToUtf16Offset(sourceCode, line, column);
    if (utf16Offset === undefined) {
      throw new Error('Failed to convert line and column to UTF-16 offset');
    }
    return utf16Offset;
  }

  getColAndLineByOffset(filename: String, offset: number): LspSourceLocation {
    const sourceCode = this.getFileSource(filename.valueOf());
    const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(sourceCode, offset);
    if (codePointOffset === undefined) {
      throw new Error('Failed to convert UTF-16 offset to code point offset');
    }
    const location = new LspSourceLocation(
      global.es2panda._getColAndLineByOffset(sourceCode, codePointOffset)
    );
    const utf16Column = TextPositionUtils.codePointColumnToUtf16Column(
      sourceCode,
      location.line,
      location.column
    );
    if (utf16Column === undefined) {
      throw new Error('Failed to convert code point column to UTF-16 column');
    }
    location.column = utf16Column;
    return location;
  }

  private convertTextSpanToUtf16(source: string, span: TextSpan): void {
    const start = TextPositionUtils.codePointOffsetToUtf16Offset(source, span.start);
    const end = TextPositionUtils.codePointOffsetToUtf16Offset(source, span.start + span.length);
    if (start === undefined || end === undefined) {
      throw new Error('Failed to convert text span to UTF-16 offsets');
    }
    span.start = start;
    span.length = end - start;
  }

  private getDefinitionFromFileCache(filename: String, offset: number): LspDefinitionData | undefined {
    return this.withFileCacheForQuery(filename, (fileCache) => {
      const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
      if (codePointOffset === undefined) {
        return undefined;
      }
      const ptr = global.es2panda._getDefinitionAtPosition(fileCache.fileContext, codePointOffset);
      const result = new LspDefinitionData(ptr);
      const nodeInfo = this.getNodeInfos(filename, result.fileName, result.start);
      if (nodeInfo !== undefined && nodeInfo.length > 0) {
        result.nodeInfos = nodeInfo;
      }
      return result;
    });
  }

  private prepareReferenceResult(filename: String, reference: LspReferenceData): void {
    const nodeInfo = this.getNodeInfos(filename, reference.fileName, reference.start);
    if (nodeInfo !== undefined && nodeInfo.length > 0) {
      reference.nodeInfos = nodeInfo;
    }
    this.convertTextSpanToUtf16(this.getFileSource(reference.fileName.valueOf()), reference);
  }

  private convertRenameLocationToUtf16(location: LspRenameLocation): void {
    const source = this.getFileSource(location.fileName);
    const start = TextPositionUtils.codePointOffsetToUtf16Offset(source, location.start);
    const end = TextPositionUtils.codePointOffsetToUtf16Offset(source, location.end);
    if (start === undefined || end === undefined) {
      throw new Error('Failed to convert rename location to UTF-16 offsets');
    }
    location.start = start;
    location.end = end;
  }

  private convertCodeFixActionsToUtf16(actions: CodeFixActionInfo[]): void {
    actions.forEach((action) => {
      action.changes.forEach((change) => {
        this.convertTextChangesToUtf16(change.fileName, change.textChanges);
      });
    });
  }

  private getTextChangeSource(filePath: string): string | undefined {
    const cachedSource = this.filesMap.get(filePath)?.fileContent || this.inMemoryFileContents.get(filePath);
    if (cachedSource !== undefined) {
      return cachedSource.replace(/\r\n/g, '\n');
    }
    try {
      const providedSource = this.getFileContent(filePath);
      if (providedSource !== undefined) {
        return providedSource.replace(/\r\n/g, '\n');
      }
    } catch (error) {
      logger.debug(`Failed to get in-memory content for ${filePath}: ${String(error)}`);
    }
    if (fs.existsSync(filePath)) {
      return fs.readFileSync(filePath, 'utf8').replace(/\r\n/g, '\n');
    }
    return undefined;
  }

  private convertTextChangesToUtf16(fileName: String, textChanges: TextChange[]): string {
    const filePath = fileName.valueOf();
    const source = this.getTextChangeSource(filePath);
    if (source === undefined) {
      if (textChanges.some((change) => change.span.start !== 0 || change.span.length !== 0)) {
        throw new Error('Cannot convert non-empty text span for a new file');
      }
      return '';
    }
    textChanges.forEach((change) => this.convertTextSpanToUtf16(source, change.span));
    return source;
  }

  private applyTextChanges(source: string, textChanges: TextChange[]): string {
    const ordered = textChanges
      .map((change, index) => ({ change, index }))
      .sort((left, right) => left.change.span.start - right.change.span.start || left.index - right.index);
    let result = '';
    let cursor = 0;
    ordered.forEach(({ change }) => {
      if (change.span.start > source.length) {
        return;
      }
      const originalStart = change.span.start;
      const start = Math.max(originalStart, cursor);
      const end = originalStart + Math.min(change.span.length, source.length - originalStart);
      if (cursor < start) {
        result += source.slice(cursor, start);
      }
      result += change.newText.valueOf();
      cursor = Math.max(cursor, end);
    });
    return result + source.slice(cursor);
  }

  private convertDiagnosticsToUtf16(source: string, diagnostics: LspDiagsNode): void {
    diagnostics.diagnostics.forEach((diagnostic) => {
      if (diagnostic.range.start.line === 0 || diagnostic.range.end.line === 0) {
        return;
      }

      const start = TextPositionUtils.codePointColumnToUtf16Column(
        source,
        diagnostic.range.start.line,
        diagnostic.range.start.character
      );
      const end = TextPositionUtils.codePointColumnToUtf16Column(
        source,
        diagnostic.range.end.line,
        diagnostic.range.end.character
      );
      if (start === undefined || end === undefined) {
        throw new Error('Failed to convert diagnostic range to UTF-16 columns');
      }
      diagnostic.range.start.character = start;
      diagnostic.range.end.character = end;
    });
  }

  getDefinitionAtPosition(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspDefinitionData | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getDefinitionAtPosition', this.perfMetricLogFilePath) : undefined;
    try {
      const definition = nodeInfos
        ? (this.getAtPositionByNodeInfos(filename, nodeInfos, 'definition') as LspDefinitionData | undefined)
        : this.getDefinitionFromFileCache(filename, offset);

      if (definition === undefined) {
        return undefined;
      }

      const definitionFileName = definition.fileName.valueOf();
      if (definitionFileName.length === 0) {
        return definition;
      }

      const source = this.getFileSource(definitionFileName);
      const start = TextPositionUtils.codePointOffsetToUtf16Offset(source, definition.start);
      const end = TextPositionUtils.codePointOffsetToUtf16Offset(source, definition.start + definition.length);
      if (start === undefined || end === undefined) {
        return undefined;
      }

      definition.start = start;
      definition.length = end - start;
      return definition;
    } catch (error) {
      logger.error('failed to getDefinitionAtPosition', error);
      return undefined;
    } finally {
      perfScope?.end();
    }
  }

  private getMergedCompileFiles(filename: String): string[] {
    const moduleInfo = this.moduleInfos[path.resolve(filename.valueOf())];
    return moduleInfo ? [...moduleInfo.compileFiles, ...moduleInfo.depModuleCompileFiles] : [];
  }

  private buildReverseModuleDeps(): Map<string, Set<string>> {
    const reverseDeps = new Map<string, Set<string>>();
    const ownerModulesByCompileFile = this.buildOwnerModulesByCompileFile(this.buildConfigs);
    for (const moduleName of Object.keys(this.buildConfigs)) {
      reverseDeps.set(moduleName, new Set());
    }
    for (const [moduleName, buildConfig] of Object.entries(this.buildConfigs)) {
      const depModuleNames = this.collectDependencyModuleNames(moduleName, buildConfig, ownerModulesByCompileFile);
      for (const depModuleName of depModuleNames) {
        reverseDeps.get(depModuleName)?.add(moduleName);
      }
    }
    return reverseDeps;
  }

  private areStringSetsEqual(first: string[], second: string[]): boolean {
    if (first.length !== second.length) {
      return false;
    }
    const left = new Set(first.map((item) => path.resolve(item)));
    if (left.size !== second.length) {
      return false;
    }
    for (const item of second) {
      if (!left.has(path.resolve(item))) {
        return false;
      }
    }
    return true;
  }

  private buildOwnerModulesByCompileFile(configs: Record<string, BuildConfig>): Map<string, Set<string>> {
    const ownersByFile = new Map<string, Set<string>>();
    for (const [moduleName, config] of Object.entries(configs)) {
      const compileFiles = config.compileFiles ?? [];
      for (const filePath of compileFiles) {
        const normalizedPath = path.resolve(filePath);
        let owners = ownersByFile.get(normalizedPath);
        if (!owners) {
          owners = new Set<string>();
          ownersByFile.set(normalizedPath, owners);
        }
        owners.add(moduleName);
      }
    }
    return ownersByFile;
  }

  private collectDependencyModuleNames(
    moduleName: string,
    config: BuildConfig | undefined,
    ownersByFile: Map<string, Set<string>>
  ): Set<string> {
    const dependencyModules = new Set<string>();
    if (!config) {
      return dependencyModules;
    }
    const depFiles = config.depModuleCompileFiles ?? [];
    for (const filePath of depFiles) {
      const depOwners = ownersByFile.get(path.resolve(filePath));
      if (!depOwners) {
        continue;
      }
      for (const depModuleName of depOwners) {
        if (depModuleName !== moduleName) {
          dependencyModules.add(depModuleName);
        }
      }
    }
    return dependencyModules;
  }

  private collectInvalidationRoots(
    prevConfigs: Record<string, BuildConfig>,
    nextConfigs: Record<string, BuildConfig>
  ): Set<string> {
    const roots = new Set<string>();
    const prevOwnersByFile = this.buildOwnerModulesByCompileFile(prevConfigs);
    const nextOwnersByFile = this.buildOwnerModulesByCompileFile(nextConfigs);
    const allModules = new Set<string>([...Object.keys(prevConfigs), ...Object.keys(nextConfigs)]);
    for (const moduleName of allModules) {
      const prev = prevConfigs[moduleName];
      const next = nextConfigs[moduleName];
      if (!prev || !next) {
        roots.add(moduleName);
        this.collectDependencyModuleNames(moduleName, prev, prevOwnersByFile).forEach((dep) => roots.add(dep));
        this.collectDependencyModuleNames(moduleName, next, nextOwnersByFile).forEach((dep) => roots.add(dep));
        continue;
      }
      const prevCompileFiles = prev.compileFiles ?? [];
      const nextCompileFiles = next.compileFiles ?? [];
      const prevDepModuleCompileFiles = prev.depModuleCompileFiles ?? [];
      const nextDepModuleCompileFiles = next.depModuleCompileFiles ?? [];
      const isCompileFilesChanged = !this.areStringSetsEqual(prevCompileFiles, nextCompileFiles);
      const isDepModuleCompileFilesChanged = !this.areStringSetsEqual(prevDepModuleCompileFiles, nextDepModuleCompileFiles);
      if (isCompileFilesChanged || isDepModuleCompileFilesChanged) {
        roots.add(moduleName);
        this.collectDependencyModuleNames(moduleName, prev, prevOwnersByFile).forEach((dep) => roots.add(dep));
        this.collectDependencyModuleNames(moduleName, next, nextOwnersByFile).forEach((dep) => roots.add(dep));
      }
    }
    return roots;
  }

  private collectReverseDependentsFromGraph(moduleName: string, graph: Map<string, Set<string>>): Set<string> {
    const visited = new Set<string>();
    const queue: string[] = [];
    const directDependents = graph.get(moduleName);
    if (directDependents) {
      queue.push(...directDependents);
    }
    for (let i = 0; i < queue.length; i++) {
      const current = queue[i];
      if (visited.has(current)) {
        continue;
      }
      visited.add(current);
      const next = graph.get(current);
      if (next) {
        queue.push(...next);
      }
    }
    return visited;
  }

  private invalidateReverseClosureCache(
    changedModules: Set<string>,
    prevReverseModuleDeps: Map<string, Set<string>>,
    nextReverseModuleDeps: Map<string, Set<string>>
  ): void {
    if (changedModules.size === 0 || this.reverseClosureCache.size === 0) {
      return;
    }
    const invalidateSet = new Set<string>();
    const graphs = [prevReverseModuleDeps, nextReverseModuleDeps];
    for (const moduleName of changedModules) {
      invalidateSet.add(moduleName);
      for (const graph of graphs) {
        const dependents = this.collectReverseDependentsFromGraph(moduleName, graph);
        dependents.forEach((name) => invalidateSet.add(name));
      }
    }
    invalidateSet.forEach((name) => this.reverseClosureCache.delete(name));
  }

  private collectReverseDependents(moduleName: string): Set<string> {
    const cached = this.reverseClosureCache.get(moduleName);
    if (cached) {
      return cached;
    }
    const visited = this.collectReverseDependentsFromGraph(moduleName, this.reverseModuleDeps);
    this.reverseClosureCache.set(moduleName, visited);
    return visited;
  }

  private getMergedCompileFilesCrossModule(filename: String): string[] {
    const curModuleInfo = this.moduleInfos[path.resolve(filename.valueOf())];
    if (!curModuleInfo || !Array.isArray(curModuleInfo.compileFiles)) {
      return [];
    }
    const resultSet = new Set<string>();
    curModuleInfo.compileFiles.forEach((file) => resultSet.add(file));
    if (Array.isArray(curModuleInfo.depModuleCompileFiles)) {
      curModuleInfo.depModuleCompileFiles.forEach((file) => resultSet.add(file));
    }

    const dependents = this.collectReverseDependents(curModuleInfo.packageName);
    dependents.forEach((dependentModuleName) => {
      const buildConfig = this.buildConfigs[dependentModuleName];
      if (!buildConfig) {
        return;
      }
      if (Array.isArray(buildConfig.compileFiles)) {
        buildConfig.compileFiles.forEach((file) => resultSet.add(file));
      }
      if (Array.isArray(buildConfig.depModuleCompileFiles)) {
        buildConfig.depModuleCompileFiles.forEach((file) => resultSet.add(file));
      }
    });

    return Array.from(resultSet);
  }

  getSemanticDiagnostics(filename: String): LspDiagsNode | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSemanticDiagnostics', this.perfMetricLogFilePath) : undefined;
    const fileName = filename.valueOf();
    const shouldRefreshInitialDiagnostics = this.pendingInitialDiagnosticFiles.has(fileName);
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const diagnostics = new LspDiagsNode(global.es2panda._getSemanticDiagnostics(fileCache.fileContext));
        this.convertDiagnosticsToUtf16(fileCache.fileContent, diagnostics);
        this.markDiagnosticFile(fileName, diagnostics);
        return diagnostics;
      } catch (error) {
        logger.error('failed to getSemanticDiagnostics by fileCache', error);
        return undefined;
      }
    }, shouldRefreshInitialDiagnostics);
    if (result) {
      this.pendingInitialDiagnosticFiles.delete(fileName);
    }
    perfScope?.end();
    return result;
  }

  getCurrentTokenValue(filename: String, offset: number): string | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getCurrentTokenValue', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return unpackString(global.es2panda._getCurrentTokenValue(fileCache.fileContext, offset));
      } catch (error) {
        logger.error('failed to getCurrentTokenValue by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getImplementationAtPosition(filename: String, offset: number): LspDefinitionData | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getImplementationAtPosition', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        const definition = new LspDefinitionData(
          global.es2panda._getImplementationAtPosition(fileCache.fileContext, codePointOffset)
        );
        const definitionFileName = definition.fileName.valueOf();
        if (definitionFileName.length > 0) {
          const source = this.getFileSource(definitionFileName);
          const span = { start: definition.start, length: definition.length };
          this.convertTextSpanToUtf16(source, span);
          definition.start = span.start;
          definition.length = span.length;
        }
        return definition;
      } catch (error) {
        logger.error('failed to getImplementationAtPosition by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getFileReferences(filename: String): LspReferenceData[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getFileReferences', this.perfMetricLogFilePath) : undefined;
    let searchFileCache = this.filesMap.get(filename.valueOf());
    let ptr: KPointer;
    const isWindows = typeof process !== 'undefined' && process.platform === 'win32';
    filename = isWindows ? filename.replace(/\//g, '\\') : filename;
    const resolvedFileName = path.resolve(filename.valueOf());
    if (searchFileCache) {
      try {
        ptr = global.es2panda._getFileReferences(resolvedFileName, searchFileCache.fileContext, false);
      } catch (error) {
        logger.error('failed to getFileReferences by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContextForCrossModule(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
        ptr = global.es2panda._getFileReferences(resolvedFileName, ctx, false);
      } catch (error) {
        logger.error('failed to getFileReferences', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    let result: LspReferenceData[] = [];
    let refs = new LspReferences(ptr);
    for (let i = 0; i < refs.referenceInfos.length; i++) {
      if (refs.referenceInfos[i].fileName !== '') {
        result.push(refs.referenceInfos[i]);
      }
    }
    perfScope?.end();
    return result;
  }

  // Collects the dynamic module names (`dynamic@ohos.xxx`) referenced by the file source.
  private collectDynamicImports(source: string): string[] {
    const dynamicImports: string[] = [];
    const pattern = /['"]dynamic@([^'"]+)['"]/g;
    let match: RegExpExecArray | null;
    while ((match = pattern.exec(source)) !== null) {
      dynamicImports.push(`dynamic@${match[1]}`);
    }
    return dynamicImports;
  }

  // The dynamic module names (`dynamic@ohos.xxx`) referenced by the given file, served from
  // the scan cache when the file content is unchanged.
  private getDynamicImports(fileName: string, source: string): string[] {
    const hash = this.computeContentHash(source);
    const cachedScan = this.dynamicImportScanCache.get(fileName);
    if (cachedScan !== undefined && cachedScan.hash === hash) {
      return cachedScan.imports;
    }
    const dynamicImports = this.collectDynamicImports(source);
    this.dynamicImportScanCache.set(fileName, { hash, imports: dynamicImports });
    return dynamicImports;
  }

  // The arktsconfig 'dependencies' section of the given config, served from the parse
  // cache; undefined when the config cannot be read or parsed.
  private getDynamicDeclDependencies(arktsConfigFile: string): Record<string, { path?: string }> | undefined {
    const cachedDependencies = this.dynamicDeclDependenciesCache.get(arktsConfigFile);
    if (cachedDependencies !== undefined) {
      return cachedDependencies;
    }
    try {
      const parsed = JSON5.parse(fs.readFileSync(arktsConfigFile, 'utf-8'));
      const dependencies = parsed?.compilerOptions?.dependencies ?? {};
      this.dynamicDeclDependenciesCache.set(arktsConfigFile, dependencies);
      return dependencies;
    } catch (error) {
      logger.warn(`failed to parse arktsconfig for dynamic decls: ${arktsConfigFile}`);
      return undefined;
    }
  }

  // Builds the symbol reference index of one dynamic SDK declaration file. Checked state
  // resolves the declaration scopes (GetOwner), which the index registration relies on.
  private indexDynamicDeclFile(declPath: string): void {
    const [declFileCfg, declFileCtx] = this.createContext(declPath) ?? [];
    if (!declFileCfg || !declFileCtx) {
      return;
    }
    try {
      global.es2panda._buildSymbolReferenceIndexForContext(declFileCtx);
      this.indexedDynamicDecls.add(declPath);
    } catch (error) {
      logger.warn(`build symbol reference index failed for dynamic decl ${declPath}: ${String(error)}`);
    } finally {
      this.destroyContext(declFileCfg, declFileCtx);
    }
  }

  // Dynamic imports are foreign bindings: their interop declaration files are not parsed
  // into the context externals, so the symbol reference index never sees them. Index the
  // declaration files of the dynamic modules imported by the given file explicitly, so
  // reference queries also report references inside the dynamic SDK declarations.
  private indexDynamicSdkDecls(fileName: String): void {
    const moduleInfo = this.moduleInfos[path.resolve(fileName.valueOf())];
    const arktsConfigFile = moduleInfo?.arktsConfigFile;
    if (!arktsConfigFile || !fs.existsSync(arktsConfigFile)) {
      return;
    }
    const dynamicImports = this.getDynamicImports(fileName.valueOf(), this.getFileSource(fileName.valueOf()));
    if (dynamicImports.length === 0) {
      return;
    }
    const dependencies = this.getDynamicDeclDependencies(arktsConfigFile);
    if (dependencies === undefined) {
      return;
    }
    dynamicImports.forEach((moduleName) => {
      const declPath = dependencies[moduleName]?.path;
      if (!declPath || !fs.existsSync(declPath) || this.indexedDynamicDecls.has(declPath)) {
        return;
      }
      this.indexDynamicDeclFile(declPath);
    });
  }

  // Auto import entries come from the symbol export index, which only holds programs that
  // were parsed somewhere. The files a project's dynamic module lists in its decl-fileInfo
  // are not parsed unless some file imports them, so their exported symbols (a sibling
  // file nobody imports yet) would never be suggested. Index every listed declaration up
  // front so the import suggestions cover the whole module. Runs before the cache
  // context's own external indexing so the live context's registrations replace these
  // for the files it also parses.
  private indexProjectDynamicDecls(): void {
    Object.values(this.buildConfigs).forEach((buildConfig) => this.indexModuleDeclFiles(buildConfig));
  }

  private indexModuleDeclFiles(buildConfig: BuildConfig): void {
    if (!buildConfig.declFilesPath || !fs.existsSync(buildConfig.declFilesPath)) {
      return;
    }
    let declFiles: Record<string, { declPath?: string }>;
    try {
      declFiles = (JSON5.parse(fs.readFileSync(buildConfig.declFilesPath, 'utf-8'))?.files ?? {}) as Record<
        string,
        { declPath?: string }
      >;
    } catch (error) {
      logger.warn(`failed to index decl files of module ${buildConfig.packageName}: ${String(error)}`);
      return;
    }
    Object.values(declFiles).forEach((entry) => this.indexDeclFileIfNeeded(entry?.declPath));
  }

  private indexDeclFileIfNeeded(declPath?: string): void {
    if (declPath && fs.existsSync(declPath) && !this.indexedDynamicDecls.has(declPath)) {
      this.indexDynamicDeclFile(declPath);
    }
  }

  getReferencesAtPosition(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspReferenceData[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getReferencesAtPosition', this.perfMetricLogFilePath) : undefined;
    let result: LspDefinitionData[] = [];
    if (nodeInfos) {
      const anchor = this.getAtPositionByNodeInfos(filename, nodeInfos, 'reference') as LspReferenceData | undefined;
      result = anchor ? [anchor] : [];
      result.forEach((ref) => {
        this.convertTextSpanToUtf16(this.getFileSource(ref.fileName.valueOf()), ref);
      });
      perfScope?.end();
      return result;
    }

    const source = this.getFileSource(filename.valueOf());
    const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(source, offset);
    if (codePointOffset === undefined) {
      perfScope?.end();
      return undefined;
    }

    let ptr: KPointer;
    const isWindows = typeof process !== 'undefined' && process.platform === 'win32';
    filename = isWindows ? filename.replace(/\//g, '\\') : filename;
    // Index the dynamic SDK declaration files imported by this file before querying.
    this.indexDynamicSdkDecls(filename);
    if (this.filesMap.size > 0) {
      const cachedPtr = this.withFileCacheForQuery(
        filename,
        (fileCache) => global.es2panda._getReferencesAtPositionFromIndex(fileCache.fileContext, codePointOffset)
      );
      if (cachedPtr === undefined) {
        return;
      }
      ptr = cachedPtr;
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
        ptr = global.es2panda._getReferencesAtPositionFromIndex(ctx, codePointOffset);
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    const refs = new LspReferences(ptr);
    refs.referenceInfos.forEach((ref) => {
      this.prepareReferenceResult(filename, ref);
      result.push(ref);
    });

    result = Array.from(new Set(result));
    perfScope?.end();
    return result;
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
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getNodeInfos', this.perfMetricLogFilePath) : undefined;
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
      let fileCache = this.filesMap.get(paramFileName.valueOf());
      if (fileCache) {
        try {
          ptr = global.es2panda._getNodeInfosByDefinitionData(fileCache.fileContext, fileName, start);
          nodeInfos = new NativePtrDecoder().decode(ptr).map((elPeer: KNativePointer) => {
            return new LspNodeInfo(elPeer);
          });
        } catch (error) {
          logger.error('failed to getNodeInfos by fileCache', error);
          return;
        }
      } else {
        const [declFileCfg, declFileCtx] = this.createContext(fileName) ?? [];
        if (!declFileCfg || !declFileCtx) { return; }
        try {
          ptr = global.es2panda._getNodeInfosByDefinitionData(declFileCtx, fileName, start);
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
    perfScope?.end();
    return nodeInfos;
  }

  private createNodeInfoPtrs(nodeInfos: NodeInfo[]): KPointer[] {
    const nodeInfoPtrs: KPointer[] = [];
    nodeInfos.forEach((nodeInfo) => {
      const kind = NODE_INFO_KIND_CONVERSIONS.get(nodeInfo.kind) ?? nodeInfo.kind;
      if (UNMATCHABLE_NODE_KINDS.has(kind)) {
        logger.error(`unmatchable node info kind ${AstNodeType[kind]} for name ${nodeInfo.name}`);
        return;
      }
      nodeInfoPtrs.push(global.es2panda._CreateNodeInfoPtr(nodeInfo.name, kind, nodeInfo.paramNumber ?? -1));
    });
    return nodeInfoPtrs;
  }

  private getAtPositionByNodeInfos(
    declFilePath: String,
    nodeInfos: NodeInfo[],
    type: 'definition' | 'reference' | 'renameLocation'
  ): LspNode | undefined {
    let ptr: KPointer;
    let nodeInfoPtrs: KPointer[] = this.createNodeInfoPtrs(nodeInfos);
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
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getTypeHierarchies', this.perfMetricLogFilePath) : undefined;
    let ptr: KPointer;
    let ctxFile: KPointer;
    let mainCfg: Config | undefined;
    let mainCtx: KPointer | undefined;

    try {
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
        const [cfg, ctx] = this.createContextForCrossModule(filename) ?? [];
        if (!cfg || !ctx) { return; }
        mainCfg = cfg;
        mainCtx = ctx;
        ctxFile = ctx;
        try {
          ptr = global.es2panda._getTypeHierarchies(ctxFile, ctxFile, offset);
        } catch (error) {
          logger.error('failed to getTypeHierarchies', error);
          return;
        }
      }

      let ref = new LspTypeHierarchiesInfo(ptr);
      if (ref.fileName === '') {
        return;
      }
      let result: LspTypeHierarchiesInfo[] = [];
      let compileFiles = this.getMergedCompileFilesCrossModule(filename);
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
          const [cfg, searchCtx] = this.createContextForCrossModule(compileFiles[i]) ?? [];
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
      perfScope?.end();
      return result[0];
    } finally {
      if (mainCfg && mainCtx) {
        this.destroyContext(mainCfg, mainCtx);
      }
    }
  }

  getClassHierarchyInfo(filename: String, offset: number): LspClassHierarchy | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getClassHierarchyInfo', this.perfMetricLogFilePath) : undefined;
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
      const [cfg, ctx] = this.createContextForCrossModule(filename) ?? [];
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
    const result = new LspClassHierarchy(ptr);
    perfScope?.end();
    return result;
  }

  getAliasScriptElementKind(filename: String, offset: number): LspCompletionEntryKind | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getAliasScriptElementKind', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return global.es2panda._getAliasScriptElementKind(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to getAliasScriptElementKind by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getClassHierarchies(filename: String, offset: number): LspClassHierarchies | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getClassHierarchies', this.perfMetricLogFilePath) : undefined;
    let contextList = [];
    let nativeContextList: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      nativeContextList = global.es2panda._pushBackToNativeContextVector(fileCache.fileContext, fileCache.fileContext, 1)
    } else {
      const [cfg, ctx] = this.createContextForCrossModule(filename) ?? [];
      if (!cfg || !ctx) { return; }
      contextList.push({ ctx: ctx, cfg: cfg });
      nativeContextList = global.es2panda._pushBackToNativeContextVector(ctx, ctx, 1);
    }
    let compileFiles = this.getMergedCompileFilesCrossModule(filename);
    for (let i = 0; i < compileFiles.length; i++) {
      let filePath = path.resolve(compileFiles[i]);
      if (path.resolve(filename.valueOf()) === filePath) {
        continue;
      }

      let searchFileCache = this.filesMap.get(filePath);
      if (searchFileCache) {
        global.es2panda._pushBackToNativeContextVector(searchFileCache.fileContext, nativeContextList, 0);
      } else {
        const [searchCfg, searchCtx] = this.createContextForCrossModule(filePath) ?? [];
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
    const result = new LspClassHierarchies(ptr);
    perfScope?.end();
    return result;
  }

  getClassPropertyInfo(
    filename: String,
    offset: number,
    shouldCollectInherited: boolean = false
  ): LspClassPropertyInfo | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getClassPropertyInfo', this.perfMetricLogFilePath) : undefined;
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
      const [cfg, ctx] = this.createContextForCrossModule(filename) ?? [];
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
    const result = new LspClassPropertyInfo(ptr);
    perfScope?.end();
    return result;
  }

  getOrganizeImports(filename: String): LspFileTextChanges | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getOrganizeImports', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const ptr = global.es2panda._organizeImports(fileCache.fileContext, filename);
        PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        return new LspFileTextChanges(ptr);
      } catch (error) {
        logger.error('failed to getOrganizeImports by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  findSafeDeleteLocation(filename: String, offset: number): LspSafeDeleteLocationInfo[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('findSafeDeleteLocation', this.perfMetricLogFilePath) : undefined;
    let ptr: KPointer;
    const fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._findSafeDeleteLocation(fileCache.fileContext, offset);
      } catch (error) {
        logger.error('failed to findSafeDeleteLocation by fileCache', error);
        return;
      }
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
        ptr = global.es2panda._findSafeDeleteLocation(ctx, offset);
      } catch (error) {
        logger.error('failed to findSafeDeleteLocation', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    let refs = new LspSafeDeleteLocation(ptr);
    let result: LspSafeDeleteLocationInfo[] = refs.safeDeleteLocationInfos;
    const locInfo = Array.from(new Set(result));
    perfScope?.end();
    return locInfo;
  }

  getCompletionEntryDetails(filename: String, offset: number, entryName: String): CompletionEntryDetails | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getCompletionEntryDetails', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        return new CompletionEntryDetails(
          global.es2panda._getCompletionEntryDetails(entryName, filename, fileCache.fileContext, codePointOffset)
        );
      } catch (error) {
        logger.error('failed to getCompletionEntryDetails by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getApplicableRefactors(
    filename: String,
    kind: String,
    startPos: number,
    endPos: number
  ): ApplicableRefactorItemInfo[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getApplicableRefactors', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const startCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, startPos);
        const endCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, endPos);
        if (startCodePoint === undefined || endCodePoint === undefined) {
          return undefined;
        }
        const refs = new LspApplicableRefactorInfo(global.es2panda._getApplicableRefactors(fileCache.fileContext, kind, startCodePoint, endCodePoint));
        return Array.from(new Set(refs.applicableRefactorInfo));
      } catch (error) {
        logger.error('failed to getApplicableRefactors by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
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
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getEditsForRefactor', this.perfMetricLogFilePath) : undefined;
    let up = opts?.userPrefsPtr ?? BigInt(0);
    let fmt = opts?.FormattingSettings ?? BigInt(0);
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      const startCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, start);
      const endCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, end);
      if (startCodePoint === undefined || endCodePoint === undefined) {
        return undefined;
      }
      const refactor = new LspRefactorEditInfo(
        global.es2panda._getEditsForRefactor(
          fileCache.fileContext,
          refactorName,
          actionName,
          startCodePoint,
          endCodePoint,
          up,
          fmt
        )
      );
      let refactoredRenameSource: string | undefined;
      refactor.fileTextChanges.forEach((change) => {
        const source = this.convertTextChangesToUtf16(change.fileName, change.textChanges);
        if (change.fileName.valueOf() === refactor.renameFileName) {
          refactoredRenameSource = this.applyTextChanges(source, change.textChanges);
        }
      });
      if (refactor.renameFileName.length > 0 && refactor.renameLocation > 0) {
        const source = refactoredRenameSource ?? this.getFileSource(refactor.renameFileName);
        const renameLocation = TextPositionUtils.codePointOffsetToUtf16Offset(source, refactor.renameLocation);
        if (renameLocation === undefined) {
          return undefined;
        }
        refactor.renameLocation = renameLocation;
      }
      return refactor;
    });
    perfScope?.end();
    return result;
  }

  getClassConstructorInfo(filename: String, offset: number, properties: string[]): LspClassConstructorInfo | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getClassConstructorInfo', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        const constructorInfo = new LspClassConstructorInfo(
          global.es2panda._getClassConstructorInfo(
            fileCache.fileContext,
            codePointOffset,
            passStringArray(properties)
          )
        );
        constructorInfo.constructorInfoFileTextChanges.forEach((change) => {
          const changeFileName = change.fileName.valueOf();
          const source = this.normalizePathForComparison(changeFileName) ===
            this.normalizePathForComparison(filename.valueOf())
            ? fileCache.fileContent
            : this.getFileSource(changeFileName);
          change.constructorInfoTextChanges.forEach((textChange) =>
            this.convertTextSpanToUtf16(source, textChange.span_));
        });
        return constructorInfo;
      } catch (error) {
        logger.error('failed to getClassConstructorInfo by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getSyntacticDiagnostics(filename: String): LspDiagsNode | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSyntacticDiagnostics', this.perfMetricLogFilePath) : undefined;
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
    const result = new LspDiagsNode(ptr);
    this.convertDiagnosticsToUtf16(this.getFileSource(filename.valueOf()), result);
    this.markDiagnosticFile(filename.valueOf(), result);
    perfScope?.end();
    return result;
  }

  getSuggestionDiagnostics(filename: String): LspDiagsNode | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSuggestionDiagnostics', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const diagnostics = new LspDiagsNode(global.es2panda._getSuggestionDiagnostics(fileCache.fileContext));
        this.convertDiagnosticsToUtf16(fileCache.fileContent, diagnostics);
        return diagnostics;
      } catch (error) {
        logger.error('failed to getSuggestionDiagnostics by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getQuickInfoAtPosition(filename: String, offset: number): LspQuickInfo | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getQuickInfoAtPosition', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return new LspQuickInfo(global.es2panda._getQuickInfoAtPosition(filename, fileCache.fileContext, offset));
      } catch (error) {
        logger.error('failed to getQuickInfoAtPosition by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getDocumentHighlights(filename: String, offset: number): LspDocumentHighlightsReferences | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getDocumentHighlights', this.perfMetricLogFilePath) : undefined;
    const source = this.getFileSource(filename.valueOf());
    const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(source, offset);
    if (codePointOffset === undefined) {
      return undefined;
    }
    let ptr: KPointer;
    let fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      try {
        ptr = global.es2panda._getDocumentHighlights(fileCache.fileContext, codePointOffset);
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
        global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
        ptr = global.es2panda._getDocumentHighlights(ctx, codePointOffset);
      } catch (error) {
        logger.error('failed to getDocumentHighlights', error);
        return;
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }
    const result = new LspDocumentHighlightsReferences(ptr);
    result.documentHighlights.forEach((documentHighlight) => {
      documentHighlight.highlightSpans.forEach((highlight) => {
        const highlightSource = this.getFileSource(highlight.fileName.valueOf());
        this.convertTextSpanToUtf16(highlightSource, highlight.textSpan);
        this.convertTextSpanToUtf16(highlightSource, highlight.contextSpan);
      });
    });
    perfScope?.end();
    return result;
  }

  getCompletionAtPosition(filename: String, offset: number): LspCompletionInfo | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getCompletionAtPosition', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        return new LspCompletionInfo(
          global.es2panda._getCompletionAtPosition(fileCache.fileContext, codePointOffset)
        );
      } catch (error) {
        logger.error('failed to getCompletionAtPosition by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  toLineColumnOffset(filename: String, offset: number): LspLineAndCharacter | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('toLineColumnOffset', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return new LspLineAndCharacter(global.es2panda._toLineColumnOffset(fileCache.fileContext, offset));
      } catch (error) {
        logger.error('failed to toLineColumnOffset by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getSafeDeleteInfo(filename: String, position: number): boolean | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSafeDeleteInfo', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return global.es2panda._getSafeDeleteInfo(fileCache.fileContext, position);
      } catch (error) {
        logger.error('failed to getSafeDeleteInfo by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getTokenNative(filename: String, position: number): LspTokenNativeInfo {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getTokenNative', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const typeInfo = new LspTokenTypeInfo(global.es2panda._getTokenTypes(fileCache.fileContext, position));
        if (typeof typeInfo.type === 'string' && typeInfo.type.includes('native')) {
          return new LspTokenNativeInfo(typeInfo.name, true);
        }
        return new LspTokenNativeInfo(typeInfo.name, false);
      } catch (error) {
        console.error(error);
        throw error;
      }
    });
    perfScope?.end();
    return result ?? new LspTokenNativeInfo();
  }

  // Extracts the identifier covering the given UTF-16 offset of the source: the
  // offset may point anywhere inside the identifier.
  private extractIdentifierAt(source: string, offset: number): string | undefined {
    if (offset < 0 || offset >= source.length) {
      return undefined;
    }
    let start = offset;
    while (start > 0 && /[A-Za-z0-9_$]/.test(source[start - 1])) {
      start--;
    }
    const match = source.substring(start).match(/^[A-Za-z_$][A-Za-z0-9_$]*/);
    return match !== null ? match[0] : undefined;
  }

  findRenameLocations(filename: String, offset: number, nodeInfos?: NodeInfo[]): LspRenameLocation[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('findRenameLocations', this.perfMetricLogFilePath) : undefined;
    if (nodeInfos) {
      const location =
        this.getAtPositionByNodeInfos(filename, nodeInfos, 'renameLocation') as LspRenameLocation | undefined;
      const result = location ? [location] : [];
      result.forEach((ref) => this.convertRenameLocationToUtf16(ref));
      perfScope?.end();
      return result;
    }

    let ptr: KPointer;
    const isWindows = typeof process !== 'undefined' && process.platform === 'win32';
    filename = isWindows ? filename.replace(/\//g, '\\') : filename;
    const fileCache = this.filesMap.get(filename.valueOf());
    if (fileCache) {
      const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
      if (codePointOffset === undefined) {
        return undefined;
      }
      ptr = global.es2panda._findRenameLocationsFromIndex(fileCache.fileContext, codePointOffset);
    } else {
      const [cfg, ctx] = this.createContext(filename) ?? [];
      if (!cfg || !ctx) {
        return;
      }
      try {
        const source = this.getFileSource(filename.valueOf());
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(source, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
        ptr = global.es2panda._findRenameLocationsFromIndex(ctx, codePointOffset);
      } finally {
        this.destroyContext(cfg, ctx);
      }
    }

    const result: LspRenameLocation[] = new NativePtrDecoder().decode(ptr).map((elPeer: KPointer) => {
      return new LspRenameLocation(elPeer);
    });

    // Renaming an aliased import only renames the alias occurrences: the symbol
    // reference index aggregates alias and declaration references under one symbol, so
    // the aggregated locations are filtered by the queried name and the original-name
    // locations stay untouched.
    const queriedSource = fileCache ? fileCache.fileContent : this.getFileSource(filename.valueOf());
    const queriedName = this.extractIdentifierAt(queriedSource, offset);
    // The per-file source is read once: the aggregated rename locations may contain many
    // references inside the same file.
    const refSources = new Map<string, string>();
    const getRefSource = (refFileName: String): string => {
      const key = refFileName.valueOf();
      let source = refSources.get(key);
      if (source === undefined) {
        source = this.getFileSource(key);
        refSources.set(key, source);
      }
      return source;
    };
    const sameNameResult = queriedName
      ? result.filter((ref) => {
        const refSource = getRefSource(ref.fileName);
        const refUtf16Start = TextPositionUtils.codePointOffsetToUtf16Offset(refSource, ref.start);
        return refUtf16Start !== undefined && this.extractIdentifierAt(refSource, refUtf16Start) === queriedName;
      })
      : result;

    sameNameResult.forEach((ref) => {
      const nodeInfoTemp: NodeInfo[] | undefined = this.getNodeInfos(filename, ref.fileName, ref.start);
      if (nodeInfoTemp !== undefined && nodeInfoTemp.length > 0) {
        ref.nodeInfos = nodeInfoTemp;
      }
      this.convertRenameLocationToUtf16(ref);
    });

    const uniqueResult = Array.from(new Set(sameNameResult));
    perfScope?.end();
    return uniqueResult;
  }

  getRenameInfo(filename: String, offset: number): LspRenameInfoType | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getRenameInfo', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const codePointOffset = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, offset);
        if (codePointOffset === undefined) {
          return undefined;
        }
        const ptr = global.es2panda._getRenameInfo(fileCache.fileContext, codePointOffset, this.pandaLibPath);
        const success = global.es2panda._getRenameInfoIsSuccess(ptr);
        if (!success) {
          return new LspRenameInfoFailure(global.es2panda._getRenameInfoFailure(ptr));
        }
        const renameInfo = new LspRenameInfoSuccess(global.es2panda._getRenameInfoSuccess(ptr));
        this.convertTextSpanToUtf16(fileCache.fileContent, renameInfo.triggerSpan);
        return renameInfo;
      } catch (error) {
        logger.error('failed to getRenameInfo by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getSpanOfEnclosingComment(filename: String, offset: number, onlyMultiLine: boolean): LspTextSpan | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSpanOfEnclosingComment', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return new LspTextSpan(global.es2panda._getSpanOfEnclosingComment(fileCache.fileContext, offset, onlyMultiLine));
      } catch (error) {
        logger.error('failed to getSpanOfEnclosingComment by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getCodeFixesAtPosition(
    filename: String,
    start: number,
    end: number,
    errorCodes: number[]
  ): CodeFixActionInfo[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getCodeFixesAtPosition', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const startCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, start);
        const endCodePoint = TextPositionUtils.utf16OffsetToCodePointOffset(fileCache.fileContent, end);
        if (startCodePoint === undefined || endCodePoint === undefined) {
          return undefined;
        }
        const codeFixActionInfoList = new CodeFixActionInfoList(
          global.es2panda._getCodeFixesAtPosition(
            fileCache.fileContext,
            startCodePoint,
            endCodePoint,
            new Int32Array(errorCodes),
            errorCodes.length
          )
        );
        const codeFixActionInfos: CodeFixActionInfo[] = [];
        codeFixActionInfos.push(...codeFixActionInfoList.codeFixActionInfos);
        this.convertCodeFixActionsToUtf16(codeFixActionInfos);
        // Independent fixes may propose the same import action for one diagnostic
        // (ImportFixes and FixImportSource both handle the unresolved-reference code);
        // one proposal is kept - the one carrying the declaration source metadata.
        // Actions that only share a description (for example per-occurrence UI plugin
        // suggestions) differ in their text changes and are all kept.
        const actionIdentity = (action: CodeFixActionInfo): string =>
          `${action.description.valueOf()}#${action.changes
            .map((change) =>
              `${change.fileName.valueOf()}|${change.textChanges
                .map((edit) => `${edit.span.start}:${edit.span.length}:${edit.newText.valueOf()}`)
                .join(',')}`
            )
            .join(';')}`;
        const byIdentity = new Map<string, CodeFixActionInfo>();
        codeFixActionInfos.forEach((action) => {
          const key = actionIdentity(action);
          const existing = byIdentity.get(key);
          if (existing === undefined || (existing.additionalMessage_ === '' && action.additionalMessage_ !== '')) {
            byIdentity.set(key, action);
          }
        });
        return Array.from(byIdentity.values());
      } catch (error) {
        logger.error('failed to getCodeFixesAtPosition by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  provideInlayHints(filename: String, span: TextSpan): LspInlayHint[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('provideInlayHints', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        const nativeSpan = global.es2panda._createTextSpan(span.start, span.length);
        const inlayHintList = new LspInlayHintList(global.es2panda._getInlayHintList(fileCache.fileContext, nativeSpan));
        const inlayHints: LspInlayHint[] = [];
        inlayHints.push(...inlayHintList.inlayHints);
        return inlayHints;
      } catch (error) {
        logger.error('failed to provideInlayHints by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
  }

  getSignatureHelpItems(filename: String, offset: number): LspSignatureHelpItems | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getSignatureHelpItems', this.perfMetricLogFilePath) : undefined;
    const result = this.withFileCacheForQuery(filename, (fileCache) => {
      try {
        return new LspSignatureHelpItems(global.es2panda._getSignatureHelpItems(fileCache.fileContext, offset));
      } catch (error) {
        logger.error('failed to getSignatureHelpItems by fileCache', error);
        return undefined;
      }
    });
    perfScope?.end();
    return result;
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

  public compileKits(kitfiles: string[]): void {
    if (kitfiles.length === 0) {
      logger.error('SDK directory not found.');
      return;
    }
    const ets2pandaCmd: string[] = formEts2pandaCmd(this.defaultArkTsConfig, kitfiles[0], true);
    let lspDriverHelper = new LspDriverHelper();
    let config = lspDriverHelper.createCfg(ets2pandaCmd, kitfiles[0]);
    const ctx = lspDriverHelper.createContextSimultaneousModeForLsp(config.peer, kitfiles.length, kitfiles, true);
    this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, ctx);
    this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_BOUND, ctx);
    global.es2panda._collectApiInfo(ctx);
    this.destroyContext(config, ctx);
  }

  // SDK validation keeps one simultaneous context per diagnostic state and
  // switches the active declaration file through the incremental API.
  public initSdkValidatorCache(validationFiles: string[], targetState: Es2pandaContextState): void {
    const files = Array.from(new Set(validationFiles.map((file) => path.resolve(file))));
    if (files.length === 0) {
      throw new Error('SDK validation files not found.');
    }
    if (
      targetState !== Es2pandaContextState.ES2PANDA_STATE_PARSED &&
      targetState !== Es2pandaContextState.ES2PANDA_STATE_CHECKED
    ) {
      throw new Error('SDK validator target state must be PARSED or CHECKED.');
    }
    if (this.filesMap.size !== 0) {
      throw new Error('Cannot initialize SDK validator cache after another AST cache.');
    }
    const ets2pandaCmd = formEts2pandaCmd(this.defaultArkTsConfig, files[0], true);
    const config = this.lspDriverHelper.createCfg(ets2pandaCmd, files[0]);
    let ctx: KNativePointer | undefined;
    try {
      ctx = this.lspDriverHelper.createContextSimultaneousModeForLsp(config.peer, files.length, files, true);
      const pluginContext = PluginDriver.getInstance().getPluginContext();
      pluginContext.setCodingFilePath(files[0]);
      pluginContext.setProjectConfig(config);
      pluginContext.setContextPtr(ctx);
      this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, ctx);
      PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
      if (targetState === Es2pandaContextState.ES2PANDA_STATE_CHECKED) {
        this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, ctx);
        PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
      }
      const content = this.getFileSource(files[0]);
      const ret = global.es2pandaPublic._IncrementalPrepareProgram(ctx, files[0], content, 0);
      if (ret !== 1) {
        throw new Error(`Failed to select SDK declaration file: ${files[0]}`);
      }
      this.filesMap.set(files[0], {
        fileContent: content,
        fileConfig: config,
        fileContext: ctx,
        fileHash: this.computeContentHash(content),
        arktsConfigPath: this.defaultArkTsConfig
      });
    } catch (error) {
      if (ctx !== undefined) {
        this.destroyContext(config, ctx);
      } else {
        this.lspDriverHelper.destroyConfig(config);
      }
      throw error;
    }
  }

  public prepareSdkValidatorFile(fileName: string): void {
    if (!fs.existsSync(fileName) || fs.statSync(fileName).isDirectory()) {
      throw new Error(`SDK declaration file not found: ${fileName}`);
    }
    if (this.filesMap.size !== 1) {
      throw new Error('SDK validator cache is not initialized.');
    }
    const filePath = path.resolve(fileName);
    const source = this.getFileSource(filePath);
    const hash = this.computeContentHash(source);
    const [activeFilePath, activeFileCache] = Array.from(this.filesMap.entries())[0];
    if (activeFilePath === filePath && activeFileCache.fileHash === hash) {
      return;
    }
    const ret = global.es2pandaPublic._IncrementalPrepareProgram(activeFileCache.fileContext, filePath, source, 0);
    if (ret !== 1) {
      throw new Error(`SDK declaration file was not compiled in the validator cache: ${filePath}`);
    }
    this.filesMap.clear();
    this.filesMap.set(filePath, {
      fileContent: source,
      fileConfig: activeFileCache.fileConfig,
      fileContext: activeFileCache.fileContext,
      fileHash: hash,
      arktsConfigPath: activeFileCache.arktsConfigPath
    });
  }

  // AST caching is not enabled by default.
  // Call `initAstCache` before invoking the language service interface to enable AST cache
  public initAstCache(): void {
    // Collect all the exported API in the SDK.
    this.compileKits(this.kitFiles);
    let files: string[] = Array.from(Object.keys(this.moduleInfos));
    const ets2pandaCmd: string[] = formEts2pandaCmd(this.defaultArkTsConfig, files[0], true, LSP_OPTIONS);
    let lspDriverHelper = new LspDriverHelper();
    let config = lspDriverHelper.createCfg(ets2pandaCmd, files[0]);
    const ctx = lspDriverHelper.createContextSimultaneousModeForLsp(config.peer, files.length, files, true);
    PluginDriver.getInstance().getPluginContext().setCodingFilePath(files[0]);
    PluginDriver.getInstance().getPluginContext().setProjectConfig(config);
    PluginDriver.getInstance().getPluginContext().setContextPtr(ctx);

    this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_PARSED, ctx);
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
    this.lspDriverHelper.proceedToState(Es2pandaContextState.ES2PANDA_STATE_CHECKED, ctx);
    PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

    this.indexProjectDynamicDecls();
    global.es2panda._buildSymbolReferenceIndexForContextWithExternal(ctx);
    this.compiledFileHashes.clear();
    this.diagnosticFiles.clear();
    this.pendingInitialDiagnosticFiles.clear();
    for (let i = 0; i < files.length; ++i) {
      if (!fs.existsSync(files[i]) || fs.statSync(files[i]).isDirectory()) {
        continue;
      }
      const normalizedContent = fs.readFileSync(files[i], 'utf8');
      this.compiledFileHashes.set(files[i], this.computeContentHash(normalizedContent));
      this.pendingInitialDiagnosticFiles.add(files[i]);
    }
    const content = fs.readFileSync(files[0], 'utf8').replace(/\r\n/g, '\n');
    // The simultaneous context carries no single-file source; prepare it for files[0]
    // (mirroring restoreFileCacheForQuery) so the first query hitting the cache reads a
    // valid program instead of an empty SourceCode.
    const ret = global.es2pandaPublic._IncrementalPrepareProgram(ctx, files[0], content, 0);
    if (ret !== 0 && ret !== 1) {
      logger.error('initAstCache prepare failed for ' + files[0] + ', ret: ' + ret);
      this.refreshBaselineRss('initAstCache main file prepare failed');
      return;
    }
    if (ret === 0) {
      this.proceedPreparedContext(files[0], ctx, true);
      this.tryBuildSymbolReferenceIndex(files[0], ctx);
    }
    this.filesMap.set(files[0], {
      fileContent: content,
      fileConfig: config,
      fileContext: ctx,
      fileHash: this.computeContentHash(content),
      arktsConfigPath: this.defaultArkTsConfig
    });
    // The AST cache and the main-file warm-up above are expected steady-state
    // residents: record them into the baseline so the memory guard only measures
    // growth beyond the warm state instead of re-counting the cache on every check.
    this.refreshBaselineRss('initAstCache completed');
  }

  private compileExternalProgram(jobInfo: JobInfo): void {
    PluginDriver.getInstance().initPlugins(jobInfo.buildConfig);
    let ets2pandaCmd = [
      '-',
      '--extension',
      'ets',
      '--arktsconfig',
      jobInfo.arktsConfigFile,
      ...LSP_OPTIONS
    ];
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
    PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
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
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getFormattingEditsForDocument', this.perfMetricLogFilePath) : undefined;
    const settingsPtr = this.createFormatCodeSettings(options);

    try {
      return this.withFileCacheForQuery(filename, (fileCache) => {
        try {
          const result = new LspFormattingTextChanges(global.es2panda._getFormattingEditsForDocument(fileCache.fileContext, settingsPtr));
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
          const changes = result.textChanges;
          result.dispose();
          return changes;
        } catch (error) {
          logger.error('failed to getFormattingEditsForDocument by fileCache', error);
          return undefined;
        }
      });
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
      perfScope?.end();
    }
  }

  getFormattingEditsForRange(
    filename: String,
    start: number,
    length: number,
    options?: FormatCodeSettingsOptions
  ): TextChange[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getFormattingEditsForRange', this.perfMetricLogFilePath) : undefined;
    const settingsPtr = this.createFormatCodeSettings(options);

    try {
      return this.withFileCacheForQuery(filename, (fileCache) => {
        try {
          const result = new LspFormattingTextChanges(global.es2panda._getFormattingEditsForRange(fileCache.fileContext, settingsPtr, start, length));
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
          const changes = result.textChanges;
          result.dispose();
          return changes;
        } catch (error) {
          logger.error('failed to getFormattingEditsForRange by fileCache', error);
          return undefined;
        }
      });
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
      perfScope?.end();
    }
  }

  getFormattingEditsAfterKeystroke(
    filename: String,
    position: number,
    key: string,
    options?: FormatCodeSettingsOptions
  ): TextChange[] | undefined {
    const perfScope = this.enablePerfMetric ? new PerfMetricScope('getFormattingEditsAfterKeystroke', this.perfMetricLogFilePath) : undefined;
    if (!key || key.length === 0) {
      return;
    }
    const settingsPtr = this.createFormatCodeSettings(options);
    const keyCode = key.charCodeAt(0);

    try {
      return this.withFileCacheForQuery(filename, (fileCache) => {
        try {
          const result = new LspFormattingTextChanges(
            global.es2panda._getFormattingEditsAfterKeystroke(fileCache.fileContext, settingsPtr, position, 0, keyCode)
          );
          PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
          const changes = result.textChanges;
          result.dispose();
          return changes;
        } catch (error) {
          logger.error('failed to getFormattingEditsAfterKeystroke by fileCache', error);
          return undefined;
        }
      });
    } finally {
      global.es2panda._destroyFormatCodeSettings(settingsPtr);
      perfScope?.end();
    }
  }

  private disposeCachedContexts(): void {
    const contexts = new Set<KNativePointer>();
    const configs = new Map<KNativePointer, Config>();
    this.filesMap.forEach((fileCache) => {
      contexts.add(fileCache.fileContext);
      configs.set(fileCache.fileConfig.peer, fileCache.fileConfig);
    });
    this.filesMap.clear();
    this.compiledFileHashes.clear();
    this.diagnosticFiles.clear();

    try {
      if (contexts.size > 0) {
        PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
      }
    } finally {
      contexts.forEach((context) => this.lspDriverHelper.destroyContext(context));
      configs.forEach((config) => this.lspDriverHelper.destroyConfig(config));
    }
  }

  private disposeGlobalContext(): void {
    if (this.globalLspDriverHelper === undefined) {
      return;
    }
    if (this.globalContextPtr !== undefined) {
      this.globalLspDriverHelper.destroyGlobalContext(this.globalContextPtr);
    }
    if (this.globalConfig !== undefined) {
      this.globalLspDriverHelper.destroyConfig(this.globalConfig);
    }
    this.globalLspDriverHelper.memFinalize();
  }

  public dispose(): void {
    if (this.disposed) {
      return;
    }
    this.disposed = true;
    this.clearIdleRecycleTimer();
    this.inMemoryFileContents.clear();

    this.disposeCachedContexts();
    this.disposeGlobalContext();
  }
}

function createHash(str: string): string {
  const hash = crypto.createHash('sha256');
  hash.update(str);
  return hash.digest('hex');
}
// Use AST cache end
