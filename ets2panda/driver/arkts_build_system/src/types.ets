/**
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

import { BytecodeObfuscationConfig } from './obfuscation/obfuscation_bytecode_config';
import { MergedConfig } from './obfuscation/obfuscation_merged_config';

export enum RECORD_TYPE {
  DEFAULT_TYPE = 'OFF',
  ON_TYPE = 'ON',
}

export enum BUILD_MODE {
  DEBUG = 'Debug',
  RELEASE = 'Release'
}

export enum BUILD_TYPE {
  BUILD = 'build',
  PREVIEW = 'preview'
}

export enum OHOS_MODULE_TYPE {
  HAP = 'hap',
  FEATURE = 'feature',
  SHARED = 'shared',
  HAR = 'har',
  ENTRY = 'entry'
}

export function isHarOrHsp(moduleType: OHOS_MODULE_TYPE): boolean {
  return moduleType === OHOS_MODULE_TYPE.HAR || moduleType === OHOS_MODULE_TYPE.SHARED;
}

export enum WorkerMessageType {
  DECL_GENERATED = 'DECL_GENERATED',
  ABC_COMPILED = 'ABC_COMPILED',
  ERROR_OCCURED = 'ERROR_OCCURED',
  ASSIGN_TASK = 'ASSIGN_TASK',
  TASK_FINISHED = 'TASK_FINISHED',
  LOG = 'LOG'
}

export enum LogLevel {
  INFO = 'INFO',
  WARN = 'WARN',
  DEBUG = 'DEBUG',
  ERROR = 'ERROR',
  ERROR_AND_EXIT = 'ERROR_AND_EXIT',
}

export type PluginsConfig = Map<string, string>;

export type PathsConfig = Map<string, string[]>;

export interface BuildBaseConfig {
  buildType: BUILD_TYPE;
  buildMode: BUILD_MODE;
  es2pandaMode: ES2PANDA_MODE;
  hasMainModule: boolean;
  isBuildConfigModified: boolean | undefined;
  recordType: 'OFF' | 'ON' | undefined;
  dumpDependencyGraph: boolean | undefined;
  dumpPerf: boolean | undefined;
  transformLib: string | undefined;
}

export interface ModuleConfig {
  packageName: string;
  moduleType: OHOS_MODULE_TYPE;
  moduleRootPath: string;
  sourceRoots: string[];
  byteCodeHar: boolean;
  entryFile: string;
  bundleType: string;
  bundleName: string;
}

export interface PathConfig {
  loaderOutPath: string;
  cachePath: string;
  buildSdkPath: string;
  pandaSdkPath: string | undefined;
  pandaStdlibPath: string | undefined;
  externalApiPaths: string[];
  abcLinkerPath: string | undefined;
  dependencyAnalyzerPath: string | undefined;
  sdkAliasConfigPaths: string[] | undefined;
  sdkAliasMap: Map<string, string>;
  interopSDKPaths: Set<string>;
  interopApiPaths: string[];
  projectRootPath: string;
  arkGuardPath: string | undefined;
  toolChainsPath: string | undefined;
  disAsmStaticPath: string | undefined;
  aceModuleJsonPath: string | undefined;
  aceProfilePath: string | undefined;
  aceModuleRoot: string | undefined;
}

export interface FrameworkConfig {
  frameworkMode: boolean | undefined;
  useEmptyPackage: boolean | undefined;
}

export interface DeclgenConfig {
  enableDeclgenEts2Ts: boolean;
  declgenV1OutPath: string | undefined;
  declgenV2OutPath: string;
  declgenBridgeCodePath: string | undefined;
  skipDeclCheck: boolean | undefined;
  continueOnError: boolean | undefined;
  genDeclAnnotations: boolean | undefined;
}

export interface LoggerConfig {
  getHvigorConsoleLogger: Function | undefined;
  enableDebugOutput: boolean | undefined;
}

export interface DependencyModuleConfig {
  packageName: string;
  moduleName: string;
  moduleType: string;
  modulePath: string;
  sourceRoots: string[];
  entryFile: string;
  language: string;
  declFilesPath: string | undefined;
  dependencies: string[] | undefined;
  abcPath: string | undefined;
  declgenV1OutPath: string | undefined;
  declgenV2OutPath: string | undefined;
  declgenBridgeCodePath: string | undefined;
  byteCodeHar: boolean | undefined;
  bundleType: string | undefined;
  bundleName: string | undefined;
  packageVersion: string | undefined;
  originalPackageNameMap: Map<string, string> | undefined;
}

export interface ModuleFile {
  packageName: string;
  staticFiles: string[];
}

export interface ObfuscationSystemConfig {
  obfuscationOptions: ObfuscationOptions | undefined;
  mergedObConfig: MergedConfig | undefined;
  bytecodeObfuscationConfig: BytecodeObfuscationConfig | undefined;
}

export interface RuleOptions {
  enable: boolean | undefined;
  files: string | string[] | undefined;
  rules: string[] | undefined;
}

export interface Obfuscation {
  ruleOptions: RuleOptions | undefined;
  consumerFiles: string | string[] | undefined;
  consumerRules: string[] | undefined;
  libDir: string;
}

export interface ObfuscationConfig {
  selfConfig: Obfuscation | undefined;
  dependencies: ObfuscationDependencies;
}

export interface ObfuscationDependencies {
  libraries: Obfuscation[];
  hspLibraries: Obfuscation[];
  hars: string[];
  hsps: string[];
}

export interface ModuleJsonModuleConfig {
  name: string | undefined;
  appStartup: string | undefined;
  pages: string | undefined;
  srcEntrance: string | undefined;
  srcEntry: string | undefined;
  abilities: AbilityConfig[] | undefined;
  extensionAbilities: AbilityConfig[] | undefined;
  packageName: string | undefined;
}

export interface ModuleJsonConfig {
  module: ModuleJsonModuleConfig | undefined;
}

export interface AbilityMetadataConfig {
  name: string | undefined;
  value: string | undefined;
  resource: string | undefined;
}

export interface AbilityConfig {
  srcEntry: string | undefined;
  srcEntrance: string | undefined;
  metadata: AbilityMetadataConfig[] | undefined;
}

export interface StartupTask {
  srcEntry: string | undefined;
}

export interface CardFormConfig {
  type: string | undefined;
  uiSyntax: string | undefined;
  src: string;
}

export interface StartupConfig {
  configEntry: string | undefined;
  startupTasks: StartupTask[] | undefined;
}

export interface EntryConfig {
  src: string[] | undefined;
}

export interface ObfuscationOptions extends ObfuscationConfig {
  sdkApis: string[];
  obfuscationCacheDir: string;
  exportRulePath: string;
}

export interface DeclFileNameCacheConfig {
  nameCachePath: string;
}

export interface BuildConfig extends BuildBaseConfig, DeclgenConfig, LoggerConfig, ModuleConfig, PathConfig, FrameworkConfig, ObfuscationSystemConfig {
  plugins: PluginsConfig;
  paths: PathsConfig;
  compileFiles: string[];
  dependencyModuleList: DependencyModuleConfig[];
  aliasConfig: Map<string, Map<string, AliasConfig>>;
  dependentModuleList: DependencyModuleConfig[];
  moduleFiles: ModuleFile[];
  declFileNameCacheConfig: DeclFileNameCacheConfig | undefined;
}

export interface ModuleInfo {
  isMainModule: boolean;
  packageName: string;
  moduleRootPath: string;
  moduleType: string;
  sourceRoots: string[];
  entryFile: string;
  arktsConfigFile: string;
  declgenV1OutPath: string | undefined;
  declgenV2OutPath: string | undefined;
  declgenBridgeCodePath: string | undefined;
  dependencies: string[];
  staticDependencyModules: Map<string, ModuleInfo>;
  dynamicDependencyModules: Map<string, ModuleInfo>;
  language: string | undefined;
  declFilesPath: string | undefined;
  abcPath: string | undefined;
  frameworkMode: boolean | undefined;
  useEmptyPackage: boolean | undefined;
  byteCodeHar: boolean | undefined;
  staticFiles: string[];
  moduleName: string | undefined;
  bundleType: string | undefined;
  bundleName: string | undefined;
  packageVersion: string | undefined;
  originalPackageNameMap: Map<string, string> | undefined;
}

export interface SetupClusterOptions {
  clearExitListeners: boolean | undefined;
  execPath: string | undefined;
  execArgs: string[] | undefined;
}

export interface AliasConfig {
  originalAPIName: string;
  isStatic: boolean;
}

export interface AstNode {
  kind: string;
  statements: AstNode[];
  source: LiteralNode;
  specifiers: ImportSpecifierNode[];
}

export interface LiteralNode {
  str: string;
  clone: Function;
}

export interface IdentifierNode {
  name: string;
}

export interface ImportSpecifierNode {
  imported: IdentifierNode | undefined;
}

export interface ETSImportDeclaration extends AstNode {
  specifiers: ImportSpecifierNode[];
  source: LiteralNode;
}

export enum Es2pandaImportKinds {
  IMPORT_KINDS_VALUE = 0,
}

export enum Es2pandaImportFlags {
  IMPORT_FLAGS_NONE,
}

export enum ES2PANDA_MODE {
  RUN_PARALLEL = 'parallel',
  RUN_CONCURRENT = 'concurrent',
  RUN_SIMULTANEOUS = 'simultaneous',
  RUN = 'sequential'
}

export interface DynamicFileContext {
  filePath: string;
  fileName: string;
  relativePath: string;
  isExcludedDir: boolean;
  dependencySection: Map<string, DependencyItem>;
  prefix: string | undefined;
}

export interface DependencyItem {
  language: string;
  path: string;
  ohmUrl: string;
  sourceFilePath: string | undefined;
  alias: string[] | undefined;
  mainFile: string | undefined;
}

export interface CompilerOptions {
  packageName: string;
  baseUrl: string;
  rootDir: string;
  paths: Map<string, string[]>;
  dependencies: Map<string, DependencyItem>;
  useEmptyPackage: boolean | undefined;
  cacheDir: string | undefined;
  declgenV2OutPath: string | undefined;
}

export interface ArkTSConfigObject {
  compilerOptions: CompilerOptions;
}

export enum JobContentType {
  FILE = 0b0,
  CLUSTER = 0b1,
}

export interface FileInfo {
  input: string;
  output: string;
}

export interface JobInfo {
  contentType: JobContentType;
  content: FileInfo | FileInfo[];
  arktsConfig: string;
  moduleName: string;
  moduleRoot: string;
}

export enum CompileJobType {
  NONE = 0b00,
  DECL = 0b01,
  ABC = 0b10,
  DECL_ABC = 0b11
}

export interface CompileJobInfo extends JobInfo {
  declgenConfig: DeclgenV2JobConfig;
  jobType: CompileJobType;
}

export interface ProcessCompileTask extends CompileJobInfo {
  buildConfig: BuildConfig;
}

export interface DeclgenV2JobConfig {
  output: string;
}

export interface DeclgenV1JobInfo extends JobInfo {
  fileToModuleMap: Map<string, ModuleInfo>;
}

export interface ProcessDeclgenV1Task extends DeclgenV1JobInfo {
  buildConfig: BuildConfig;
}

export interface DeclFileInfo {
  delFilePath: string;
  declLastModified: number;
  glueCodeFilePath: string;
  glueCodeLastModified: number;
  sourceFilePath: string;
  sourceFileLastModified: number;
}
