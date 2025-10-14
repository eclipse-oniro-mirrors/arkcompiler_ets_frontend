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

export enum BUILD_MODE {
    DEBUG = 'Debug',
    RELEASE = 'Release'
};

export enum BUILD_TYPE {
    BUILD = 'build',
    PREVIEW = 'preview'
}

export enum OHOS_MODULE_TYPE {
    HAP = 'hap',
    FEATURE = 'feature',
    SHARED = 'shared',
    HAR = 'har',
}

export enum WorkerMessageType {
    DECL_GENERATED = 'DECL_GENERATED',
    ABC_COMPILED = 'ABC_COMPILED',
    ERROR_OCCURED = 'ERROR_OCCURED',
    ASSIGN_TASK = 'ASSIGN_TASK'
}

// ProjectConfig begins
export interface PluginsConfig {
    [pluginName: string]: string;
}

export interface PathsConfig {
    [pathName: string]: string[];
}

export interface BuildBaseConfig {
    buildType: BUILD_TYPE;
    buildMode: BUILD_MODE;
    es2pandaMode: ES2PANDA_MODE;
    hasMainModule: boolean;
    isBuildConfigModified?: boolean;
    recordType?: 'OFF' | 'ON';
    dumpDependencyGraph?: boolean;
}

export interface ArkTSGlobal {
    filePath: string;
    config: object;
    compilerContext: {
        program: object;
        peer: object
    };
    es2panda: {
        _DestroyContext: Function;
        _MemInitialize: Function;
        _MemFinalize: Function;
        _CreateGlobalContext: Function;
        _DestroyGlobalContext: Function;
        _SetUpSoPath: Function;
    }
}

export interface ArkTS {
    Config: {
        create: Function;
        createContextGenerateAbcForExternalSourceFiles: Function;
    };
    Context: {
        createFromString: Function;
        createFromStringWithHistory: Function;
    };
    EtsScript: {
        fromContext: Function;
    };
    proceedToState: Function;
    generateTsDeclarationsFromContext: Function;
    generateStaticDeclarationsFromContext: Function;
    destroyConfig: Function;
    Es2pandaContextState: typeof Es2pandaContextState;
    MemInitialize: Function;
    MemFinalize: Function;
    CreateGlobalContext: Function;
    AstNode: AstNode;
    ETSImportDeclaration: ETSImportDeclaration;
    isEtsScript: Function;
    isImportSpecifier: Function;
    isETSImportDeclaration: Function;
    factory: {
        createEtsScript: Function;
        createImportDeclaration: Function;
        createImportSpecifier: Function;
        createLiteral: Function;
        createIdentifier: Function;
        updateEtsScript: Function;
        createStringLiteral: Function;
    };
    Es2pandaImportKinds: typeof Es2pandaImportKinds;
    Es2pandaImportFlags: typeof Es2pandaImportFlags;
}

export enum Es2pandaContextState {
    ES2PANDA_STATE_NEW = 0,
    ES2PANDA_STATE_PARSED = 1,
    ES2PANDA_STATE_BOUND = 2,
    ES2PANDA_STATE_CHECKED = 3,
    ES2PANDA_STATE_LOWERED = 4,
    ES2PANDA_STATE_ASM_GENERATED = 5,
    ES2PANDA_STATE_BIN_GENERATED = 6,
    ES2PANDA_STATE_ERROR = 7
}

export interface ModuleConfig {
    packageName: string;
    moduleType: OHOS_MODULE_TYPE;
    moduleRootPath: string;
    sourceRoots: string[];
    byteCodeHar: boolean;
    entryFile: string;
}

export interface PathConfig {
    loaderOutPath: string;
    cachePath: string;
    buildSdkPath: string;
    pandaSdkPath?: string; // path to panda sdk lib/bin, for local test
    pandaStdlibPath?: string; // path to panda sdk stdlib, for local test
    externalApiPaths: string[];
    abcLinkerPath?: string;
    dependencyAnalyzerPath?: string;
    sdkAliasConfigPaths?: string[];
    sdkAliasMap: Map<string, string>;
    interopSDKPaths: Set<string>;
    interopApiPaths: string[];
    projectRootPath: string;
}

/**
 * Configuration for framework mode compilation using generate_static_abc gni.
 *
 * In framework mode, the compiler generates static ABC files from framework SDK ETS files.
 * This mode requires additional arktsconfig.json parameters for proper operation.
 */
export interface FrameworkConfig {
    /**
     * Enables or disables framework compilation mode.
     * When enabled (true), activates special processing rules for framework-level
     * compilation, including different output locations and packaging requirements.
     */
    frameworkMode?: boolean;

    /**
     * Determines whether an empty package name should be used.
     * Must be set to true when compiling framework components without a package name.
     */
    useEmptyPackage?: boolean;
}

export interface DeclgenConfig {
    enableDeclgenEts2Ts: boolean;
    declgenV1OutPath?: string;
    declgenV2OutPath?: string;
    declgenBridgeCodePath?: string;
    skipDeclCheck?: boolean;
    continueOnError?: boolean;
    genDeclAnnotations?: boolean;
}

export interface LoggerConfig {
    getHvigorConsoleLogger?: Function;
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
}

export interface BuildConfig extends BuildBaseConfig, DeclgenConfig, LoggerConfig, ModuleConfig, PathConfig, FrameworkConfig {
    plugins: PluginsConfig;
    paths: PathsConfig; // paths config passed from template to generate arktsconfig.json "paths" configs.
    compileFiles: string[];
    dependencyModuleList: DependencyModuleConfig[];
    aliasConfig: Record<string, Record<string, AliasConfig>>;
    // NOTE: left to be backward compatible with old version of build config
    // TO BE REMOVED!!
    dependentModuleList: DependencyModuleConfig[];
}
// ProjectConfig ends

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
}

export type SetupClusterOptions = {
    clearExitListeners?: boolean;
    execPath?: string;
    execArgs?: string[];
};

export type KPointer = number | bigint;

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
    imported?: IdentifierNode;
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
    RUN_PARALLEL = "parallel",
    RUN_CONCURRENT = "concurrent",
    RUN_SIMULTANEOUS = "simultaneous",
    RUN = "sequential"
};

export interface DynamicFileContext {
    filePath: string;
    fileName: string;
    relativePath: string;
    isExcludedDir: boolean;
    dependencySection: Record<string, DependencyItem>;
    prefix?: string;
}

export interface DependencyItem {
    language: string,
    path: string,
    ohmUrl: string,
    sourceFilePath?: string,
    alias?: string[]
}

export interface ArkTSConfigObject {
    compilerOptions: {
        package: string,
        baseUrl: string,
        paths: Record<string, string[]>;
        dependencies: Record<string, DependencyItem>;
        useEmptyPackage?: boolean;
        projectRootPath?: string,
        cacheDir?: string,
    }
};

export interface JobInfo {
    id: string;
    fileList: string[];
    jobDependencies: string[];
    jobDependants: string[];
}

export interface FileInfo {
    input: string;
    output: string;
    arktsConfig: string;
    moduleName: string;
    moduleRoot: string;
};

export enum CompileJobType {
    NONE        = 0b00,
    DECL        = 0b01,
    ABC         = 0b10,
    DECL_ABC    = 0b11
}

export interface CompileJobInfo extends JobInfo {
    fileInfo: FileInfo,
    declgenConfig: DeclgenV2JobConfig;
    type: CompileJobType
}

export interface ProcessCompileTask extends CompileJobInfo {
    buildConfig: BuildConfig;
}

export interface DeclgenV1JobConfig {
    otuput: string;
    bridgeCode: string;
}

export interface DeclgenV2JobConfig {
    output: string;
}

export interface DeclgenV1JobInfo extends JobInfo {
    fileInfo: FileInfo,
    declgenConfig: DeclgenV1JobConfig
}

export interface ProcessDeclgenV1Task extends DeclgenV1JobInfo {
    buildConfig: BuildConfig;
}
