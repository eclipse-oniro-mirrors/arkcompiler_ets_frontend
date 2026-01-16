
# Build-config

This document describes fields of build-config


### Build-config's structure

Build-config consists from several config structures:

| Structure | Structure fields |
|--------------|----------------|
| **BuildBaseConfig** | `buildType: BUILD_TYPE`<br>`buildMode: BUILD_MODE`<br>`es2pandaMode: ES2PANDA_MODE`<br>`hasMainModule: boolean`<br>`isBuildConfigModified?: boolean`<br>`recordType?: 'OFF' \| 'ON'`<br>`dumpDependencyGraph?: boolean`<br>`dumpPerf?: boolean` |
| **DeclgenConfig** | `enableDeclgenEts2Ts: boolean`<br>`declgenV1OutPath?: string`<br>`declgenV2OutPath: string`<br>`declgenBridgeCodePath?: string`<br>`skipDeclCheck?: boolean`<br>`continueOnError?: boolean`<br>`genDeclAnnotations?: boolean` |
| **LoggerConfig** | `getHvigorConsoleLogger?: Function` |
| **ModuleConfig** | `packageName: string`<br>`moduleType: OHOS_MODULE_TYPE`<br>`moduleRootPath: string`<br>`sourceRoots: string[]`<br>`byteCodeHar: boolean`<br>`entryFile: string` |
| **PathConfig** | `loaderOutPath: string`<br>`cachePath: string`<br>`buildSdkPath: string`<br>`pandaSdkPath?: string`<br>`pandaStdlibPath?: string`<br>`externalApiPaths: string[]`<br>`abcLinkerPath?: string`<br>`dependencyAnalyzerPath?: string`<br>`sdkAliasConfigPaths?: string[]`<br>`sdkAliasMap: Map<string, string>`<br>`interopSDKPaths: Set<string>`<br>`interopApiPaths: string[]`<br>`projectRootPath: string` |
| **FrameworkConfig** | `frameworkMode?: boolean`<br>`useEmptyPackage?: boolean` |

Futhermore, it has own fields
| Structure | Structure fields |
|-----------|------------------|
| **BuildConfig** | `plugins: PluginsConfig;`<br>`paths: PathsConfig;`<br>`compileFiles: string[];`<br>`dependencyModuleList: DependencyModuleConfig[];`<br>`aliasConfig: Record<string, Record<string, AliasConfig>>;`<br>`dependentModuleList: DependencyModuleConfig[];` |

### Build-config's fields

Required fields:

| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "plugins" | - | [pluginName: string]: string | Array of plugins and path to them |
| "paths" | - | [pathName: string]: string[] | Paths config passed from template to generate arktsconfig.json "paths" configs. |
| "compileFiles" | + | string[] | Paths to compile files |
| "dependencyModuleList" | + | DependencyModuleConfig[]* | Dependency module list consist of modules and their features |
| "aliasConfig" | - |  Record<string, Record<string, AliasConfig>> | Because of problems with naming in includes in v1.1 and v1.2 aliases were added. |
| "dependentModuleList" | - | DependencyModuleConfig[] | Left to be backward compatible with old version of build config |

DependencyModuleConfig* consists
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "packageName" | + |string | Package name |
| "moduleName" | + | string | Often the same as `packageName` |
| "moduleType" | + | string | Set the type of module. <br>'hap' is a Harmony Ability Package (HAP) is the basic unit for installing and running applications. A HAP is a module package consisting of code, resource files, third-party libraries, and an application configuration file. <br>'har' is a Harmony Archive(HAR) is a static shared package that can contain code, C++ libraries, resource files, and configuration files (also called profiles). It enables modules and projects to share code of ArkUI components, resources, and more.. It should be OHOS_MODULE_TYPE('hap', 'har', 'feature', 'shared') |
| "modulePath" | + | string | Absolute path to module directory |
| "sourceRoots" | + | string[] | Relative paths to source root directories. Resolves via modulePath |
| "entryFile" | + | string | Path to entry file |
| "language" | + | string | Could be "1.2"(if used ArkTs v.1.2), "1.1"(ArkTs v.1.1), "hybrid"(if both used) |
| "declFilesPath" | - | string | Path to declaration file(.json) |
| "dependencies" | - | string[] | Dependencies's names |
| "abcPath" | - | string | Path to .abc file |
| "declgenV1OutPath" | - | string | Path to output directory for declgen v1 |
| "declgenV2OutPath" | - | string | Path to output direcory for declgen v2 |
| "declgenBridgeCodePath" | - | string | Path to directory declgen bridge |
| "byteCodeHar" | - | boolean | The abc of the dependent bytecode har needs to be included when compiling hsp/hap, but it's not required when compiling har. Used in collectAbcFileFromByteCodeHar() |

Fields from BuildBaseConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "buildType" | + | BUILD_TYPE('build' or 'preview') | Default is 'preview' <br>'preview' build project <br>'build' build and compile abc file |
| "buildMode" | + | BUILD_MODE('Debug' or 'Release') | 'Debug' add flags '--debug-info' and '--opt-level=0' |
| "es2pandaMode" | + | ES2PANDA_MODE('parallel', 'concurrent', 'simultaneous' or 'sequential') | Set the mode of es2panda run. Default mode is 'parallel'<br>'parallel': Executes tasks using multiple processes<br>'concurrent': Executes tasks using multiple threads with ast-cache <br>'simultaneous': Build with specific es2panda mode 'simultaneous' <br>'sequential': Executes tasks sequentially in a single process and single thread
| "hasMainModule" | - | boolean | If 'hasMainModule' is true, but 'mainPackageName' or 'mainModuleRootPath' or 'mainSourceRoots' is empty, it returns error 'Main module info is not correct.' |
| "isBuildConfigModified" | - | boolean | Sets in function 'checkCacheProjectConfig' as a result of comparing existing config and current build config. |
| "recordType" | - | 'OFF' or 'ON' | The switch of performance analysis |
| "dumpDependencyGraph" | - | boolean | Write .dot files with dependency graph dump in cacheDir |
| "dumpPerf" | - | boolean | If true add flag '--dump-perf-metrics' |


Fields from DeclgenConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "enableDeclgenEts2Ts" | + | boolean | Generate declaration V1 |
| "declgenV1OutPath" | + | string | Path for output for declgen v1 |
| "declgenV2OutPath" | - | string | Path for output for declgen v2 |
| "declgenBridgeCodePath" | + | string | Path to directory declgen bridge |
| "skipDeclCheck" | - | boolean | Skips declgen checks |
| "continueOnError" | - | boolean | Not used in build_config |
| "genDeclAnnotations" | - | boolean | Generate declgen annotations |

Fields from LoggerConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "getHvigorConsoleLogger" | - | Function | Can't be initialized via build_config.json. It's used for passing logger function from hvigor. |

Fields from ModuleConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "packageName" | - | string | Package name |
| "moduleType" | - | OHOS_MODULE_TYPE('hap', 'feature', 'shared' or 'har') | Set the type of module. 'hap' is a Harmony Ability Package (HAP) is the basic unit for installing and running applications. A HAP is a module package consisting of code, resource files, third-party libraries, and an application configuration file. <br>'har' is a Harmony Archive(HAR) is a static shared package that can contain code, C++ libraries, resource files, and configuration files (also called profiles). It enables modules and projects to share code of ArkUI components, resources, and more..|
| "moduleRootPath" | + | string | Absolute path to main module |
| "sourceRoots" | + | string[] | Array of relative paths to source root directories. Resolves via moduleRootPath. |
| "byteCodeHar" | - | boolean | If it is dependent bytecode har, it is true, else false |
| "entryFile" | - | string | Absolute path to entry file |

Fields from PathConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "loaderOutPath" | + | string | Path used for --output in linker(ark_link) |
| "cachePath" | + | string | Path to cache directory. For example there could be dumps of dependency graphs and project build config file. |
| "buildSdkPath" | + | string | Uses for resolving default interopApiPaths and other relative paths |
| "pandaSdkPath" | - | string | Path to panda sdk lib/bin, for local test |
| "pandaStdlibPath" | - | string | Path to panda sdk stdlib, for local test |
| "externalApiPaths" | - | string[] | Paths to external api directories |
| "abcLinkerPath" | - | string | Path to bin/ark_link. Resolves in code via 'pandaSdkPath'. (calculates: pandaSdkPath + 'bin' + 'ark_link.exe') |
| "dependencyAnalyzerPath" | - | string | Path to bin/dependency_analyzer. Resolves in code via 'pandaSdkPath'. (calculates: pandaSdkPath + 'bin' + 'dependency_analyzer.exe') |
| "sdkAliasConfigPaths" | - | string[] | This parameter is unused |
| "sdkAliasMap" | - | Map<string, string> | Map of package-name and path to sdk alias config. |
| "interopSDKPaths" | - | Set<string> | Fills in the code. May consist api, arkts, kits, component (calculates as a set of api, arkts, kits, component paths) |
| "interopApiPaths" | - | string[] | Used for resolving interopSDKPaths. Default is '../dynamic/build-tools/interop' |
| "projectRootPath" | - | string | Compiler option in ArkTSConfig(projectRootPath) |

Fields from FrameworkConfig:
| Field | Required | Arguments | Description |
|-------|----------|-----------|-------------|
| "frameworkMode" | - | boolean | Enables or disables framework compilation mode.<br> When enabled (true), activates special processing rules for framework-level<br> compilation, including different output locations and packaging <br>requirements. |
| "useEmptyPackage" | - | boolean | Determines whether an empty package name should be used.<br>Must be set to true when compiling framework components without a <br>package name. |

