import { EtsInt, EtsNativePointer, nullptr } from '@arkts-bindings';

export enum RECORD_TYPE {
  DEFAULT_TYPE = 'OFF',
  ON_TYPE = 'ON',
}

export enum BUILD_MODE {
  DEBUG = 'Debug',
  RELEASE = 'Release',
}

export enum BUILD_TYPE {
  BUILD = 'build',
  PREVIEW = 'preview',
}

export enum OHOS_MODULE_TYPE {
  HAP = 'hap',
  FEATURE = 'feature',
  SHARED = 'shared',
  HAR = 'har',
  ENTRY = 'entry',
}

export enum ES2PANDA_MODE {
  RUN_PARALLEL = 'parallel',
  RUN_CONCURRENT = 'concurrent',
  RUN_SIMULTANEOUS = 'simultaneous',
  RUN = 'sequential',
}

export enum WorkerMessageType {
  DECL_GENERATED = 'DECL_GENERATED',
  ABC_COMPILED = 'ABC_COMPILED',
  ERROR_OCCURED = 'ERROR_OCCURED',
  ASSIGN_TASK = 'ASSIGN_TASK',
  TASK_FINISHED = 'TASK_FINISHED',
  LOG = 'LOG',
}

export enum LogLevel {
  INFO = 'INFO',
  WARN = 'WARN',
  DEBUG = 'DEBUG',
  ERROR = 'ERROR',
  ERROR_AND_EXIT = 'ERROR_AND_EXIT',
}

export enum Es2pandaContextState {
  ES2PANDA_STATE_NEW = 0,
  ES2PANDA_STATE_PARSED = 1,
  ES2PANDA_STATE_BOUND = 2,
  ES2PANDA_STATE_CHECKED = 3,
  ES2PANDA_STATE_LOWERED = 4,
  ES2PANDA_STATE_ASM_GENERATED = 5,
  ES2PANDA_STATE_BIN_GENERATED = 6,
  ES2PANDA_STATE_ERROR = 7,
}

export enum Es2pandaImportKinds {
  IMPORT_KINDS_VALUE = 0,
}

export enum Es2pandaImportFlags {
  IMPORT_FLAGS_NONE = 0,
}

export enum JobContentType {
  FILE = 0,
  CLUSTER = 1,
}

export enum CompileJobType {
  NONE = 0,
  DECL = 1,
  ABC = 2,
  DECL_ABC = 3,
}

export function isHarOrHsp(moduleType: OHOS_MODULE_TYPE): boolean {
  return moduleType === OHOS_MODULE_TYPE.HAR || moduleType === OHOS_MODULE_TYPE.SHARED;
}

export interface PluginsConfig {
  plugins: Map<string, string>;
}

export interface PathsConfig {
  paths: Map<string, string[]>;
}

export interface BuildBaseConfig {
  buildType?: BUILD_TYPE;
  buildMode?: BUILD_MODE;
  es2pandaMode?: ES2PANDA_MODE;
  hasMainModule?: boolean;
  isBuildConfigModified?: boolean;
  recordType?: RECORD_TYPE;
  dumpDependencyGraph?: boolean;
  dumpPerf?: boolean;
  transformLib?: string;
}

export interface ModuleConfig {
  packageName: string;
  moduleType?: OHOS_MODULE_TYPE;
  moduleRootPath?: string;
  sourceRoots?: string[];
  byteCodeHar?: boolean;
  entryFile?: string;
  bundleType?: string;
  bundleName?: string;
}

export interface PathConfig {
  loaderOutPath?: string;
  cachePath?: string;
  buildSdkPath?: string;
  pandaSdkPath?: string;
  pandaStdlibPath?: string;
  externalApiPaths?: string[];
  abcLinkerPath?: string;
  dependencyAnalyzerPath?: string;
  nativeLibraryPath?: string;
  sdkAliasConfigPaths?: string[];
  sdkAliasMap?: Map<string, string>;
  interopSDKPaths?: Set<string>;
  interopApiPaths?: string[];
  projectRootPath?: string;
  arkGuardPath?: string;
  toolChainsPath?: string;
  disAsmStaticPath?: string;
  aceModuleJsonPath?: string;
  aceProfilePath?: string;
  aceModuleRoot?: string;
}

export interface FrameworkConfig {
  frameworkMode?: boolean;
  useEmptyPackage?: boolean;
}

export interface DeclgenConfig {
  enableDeclgenEts2Ts?: boolean;
  declgenV1OutPath?: string;
  declgenV2OutPath?: string;
  declgenBridgeCodePath?: string;
  skipDeclCheck?: boolean;
  continueOnError?: boolean;
  genDeclAnnotations?: boolean;
}

export interface LoggerConfig {
  enableDebugOutput?: boolean;
}

export interface RuleOptions {
  enable?: boolean;
  files?: string[];
  rules?: string[];
}

export interface Obfuscation {
  ruleOptions?: RuleOptions;
  consumerFiles?: string[];
  consumerRules?: string[];
  libDir?: string;
}

export interface ObfuscationDependencies {
  libraries?: Obfuscation[];
  hspLibraries?: Obfuscation[];
  hars?: string[];
  hsps?: string[];
}

export interface ObfuscationConfig {
  selfConfig?: Obfuscation;
  dependencies?: ObfuscationDependencies;
}

export interface ObfuscationOptions extends ObfuscationConfig {
  sdkApis?: string[];
  obfuscationCacheDir?: string;
  exportRulePath?: string;
}

export interface ObfuscationSystemConfig {
  obfuscationOptions?: ObfuscationOptions;
}

export interface DependencyModuleConfig {
  packageName: string;
  moduleName: string;
  moduleType: string;
  modulePath: string;
  sourceRoots: string[];
  entryFile: string;
  language: string;
  declFilesPath?: string;
  dependencies?: string[];
  abcPath?: string;
  declgenV1OutPath?: string;
  declgenV2OutPath?: string;
  declgenBridgeCodePath?: string;
  byteCodeHar?: boolean;
  bundleType?: string;
  bundleName?: string;
  packageVersion?: string;
  originalPackageNameMap?: Map<string, string>;
}

export interface ModuleFile {
  packageName: string;
  staticFiles: string[];
}

export interface AliasConfig {
  originalAPIName: string;
  isStatic: boolean;
}

export interface DeclFileNameCacheConfig {
  nameCachePath: string;
}

export interface BuildConfig extends BuildBaseConfig, DeclgenConfig, LoggerConfig, ModuleConfig, PathConfig, FrameworkConfig, ObfuscationSystemConfig {
  externalProjectConfig?: string;
  externalProjectPath?: string;
  processWorker?: boolean;
  threadWorker?: boolean;
  useProcessWorker?: boolean;
  useThreadWorker?: boolean;
  plugins?: Map<string, string>;
  paths?: Map<string, string[]>;
  compileFiles?: string[];
  dependencyModuleList?: DependencyModuleConfig[];
  aliasConfig?: Map<string, Map<string, AliasConfig>>;
  dependentModuleList?: DependencyModuleConfig[];
  moduleFiles?: ModuleFile[];
  declFileNameCacheConfig?: DeclFileNameCacheConfig;
}

export interface ModuleInfo {
  isMainModule: boolean;
  packageName: string;
  moduleRootPath: string;
  moduleType: string;
  sourceRoots: string[];
  entryFile: string;
  arktsConfigFile: string;
  declgenV1OutPath?: string;
  declgenV2OutPath?: string;
  declgenBridgeCodePath?: string;
  dependencies: string[];
  staticDependencyModules: Map<string, ModuleInfo>;
  dynamicDependencyModules: Map<string, ModuleInfo>;
  language?: string;
  declFilesPath?: string;
  abcPath?: string;
  frameworkMode?: boolean;
  useEmptyPackage?: boolean;
  byteCodeHar?: boolean;
  staticFiles: string[];
  moduleName?: string;
  bundleType?: string;
  bundleName?: string;
  packageVersion?: string;
  originalPackageNameMap?: Map<string, string>;
}

export interface DependencyItem {
  language: string;
  path: string;
  ohmUrl: string;
  sourceFilePath?: string;
  alias?: string[];
  mainFile?: string;
}

export interface CompilerOptions {
  packageName: string;
  baseUrl: string;
  rootDir: string;
  paths: Map<string, string[]>;
  dependencies: Map<string, DependencyItem>;
  useEmptyPackage?: boolean;
  cacheDir?: string;
  declgenV2OutPath?: string;
}

export interface ArkTSConfigObject {
  compilerOptions: CompilerOptions;
  files?: string[];
}

export interface DynamicFileContext {
  filePath: string;
  fileName: string;
  relativePath: string;
  isExcludedDir: boolean;
  dependencySection: Map<string, DependencyItem>;
  prefix?: string;
}

export interface FileInfo {
  input: string;
  output: string;
}

export interface JobInfo {
  contentType: JobContentType;
  content: FileInfo[];
  arktsConfig: string;
  moduleName: string;
  moduleRoot: string;
}

export interface DeclgenV2JobConfig {
  output: string;
}

export interface CompileJobInfo extends JobInfo {
  declgenConfig: DeclgenV2JobConfig;
  jobType: CompileJobType;
}

export interface ProcessCompileTask extends CompileJobInfo {
  buildConfig: BuildConfig;
  nativeConfig?: EtsNativePointer;
  nativeContext?: EtsNativePointer;
  attempt?: EtsInt;
}

export interface DeclgenV1JobInfo extends JobInfo {
  fileToModuleMap: Map<string, ModuleInfo>;
}

export interface ProcessDeclgenV1Task extends DeclgenV1JobInfo {
  buildConfig: BuildConfig;
}

export interface DeclFileInfo {
  delFilePath: string;
  declLastModified?: number;
  glueCodeFilePath: string;
  glueCodeLastModified?: number;
  sourceFilePath: string;
  sourceFileLastModified?: number;
}

export interface ModuleJsonModuleConfig {
  name?: string;
  appStartup?: string;
  pages?: string;
  srcEntrance?: string;
  srcEntry?: string;
  abilities?: AbilityConfig[];
  extensionAbilities?: AbilityConfig[];
  packageName?: string;
}

export interface ModuleJsonConfig {
  module?: ModuleJsonModuleConfig;
}

export interface AbilityMetadataConfig {
  name?: string;
  value?: string;
  resource?: string;
}

export interface AbilityConfig {
  srcEntry?: string;
  srcEntrance?: string;
  metadata?: AbilityMetadataConfig[];
}

export interface StartupTask {
  srcEntry?: string;
}

export interface CardFormConfig {
  type?: string;
  uiSyntax?: string;
  src: string;
}

export interface StartupConfig {
  configEntry?: string;
  startupTasks?: StartupTask[];
}

export interface EntryConfig {
  src?: string[];
}

export interface LiteralNode {
  str: string;
}

export interface IdentifierNode {
  name: string;
}

export interface ImportSpecifierNode {
  imported?: IdentifierNode;
}

export interface AstNode {
  kind: string;
  statements: AstNode[];
  source?: LiteralNode;
  specifiers?: ImportSpecifierNode[];
}

export interface ETSImportDeclaration extends AstNode {
  specifiers: ImportSpecifierNode[];
  source: LiteralNode;
}

export const BUILD_CONFIG_NATIVE_HANDLE: EtsNativePointer = nullptr;
