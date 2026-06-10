import { LogDataFactory } from '../logger';
import { API, ARKTS, COMPONENT, KITS, LANGUAGE_VERSION, PANDA_SDK_PATH_FROM_SDK, PANDA_STDLIB_PATH_FROM_SDK } from '../pre_define';
import { AliasConfig, BUILD_MODE, BuildConfig, BUILD_TYPE, DependencyModuleConfig, ES2PANDA_MODE, OHOS_MODULE_TYPE, RECORD_TYPE } from '../types';
import { DriverError, ErrorCode } from '../util/error';
import { ensureFileExists, ensurePathExists, fileExists, readTextFile, resolveNativeCompilerLibraryPath, substituteEnvVarsInJSON } from '../util/interop_helper';
import { dirname, isAbsolutePath, resolvePath } from '../util/utils';

type RawObject = Record<string, Object>;

function driverError(code: string, description: string, cause: string = '', position: string = ''): DriverError {
  return new DriverError(LogDataFactory.newDetailedInstance(code, description, cause, position, [], ''));
}

function hasKey(raw: RawObject, key: string): boolean {
  return Object.hasOwn(raw, key);
}

function getString(raw: RawObject, key: string, defaultValue: string = ''): string {
  if (!hasKey(raw, key)) {
    return defaultValue;
  }
  return raw[key] as string;
}

function getBoolean(raw: RawObject, key: string, defaultValue: boolean = false): boolean {
  if (!hasKey(raw, key)) {
    return defaultValue;
  }
  return raw[key] as boolean;
}

function getStringArray(raw: RawObject, key: string): string[] {
  if (!hasKey(raw, key)) {
    return [];
  }
  let values: Object[] = raw[key] as Object[];
  let result: string[] = [];
  for (let i: int = 0; i < values.length; i++) {
    result.push(values[i].toString());
  }
  return result;
}

function optionalString(raw: RawObject, key: string): string | undefined {
  if (!hasKey(raw, key)) {
    return undefined;
  }
  return getString(raw, key);
}

function mapObjectToStringMap(value: Object | undefined): Map<string, string> {
  let result: Map<string, string> = new Map<string, string>();
  if (value === undefined) {
    return result;
  }
  let raw: RawObject = value as RawObject;
  let keys: string[] = Object.keys(raw);
  for (let i: int = 0; i < keys.length; i++) {
    result.set(keys[i], raw[keys[i]].toString());
  }
  return result;
}

function mapObjectToStringArrayMap(value: Object | undefined): Map<string, string[]> {
  let result: Map<string, string[]> = new Map<string, string[]>();
  if (value === undefined) {
    return result;
  }
  let raw: RawObject = value as RawObject;
  let keys: string[] = Object.keys(raw);
  for (let i: int = 0; i < keys.length; i++) {
    let values: Object[] = raw[keys[i]] as Object[];
    let strings: string[] = [];
    for (let j: int = 0; j < values.length; j++) {
      strings.push(values[j].toString());
    }
    result.set(keys[i], strings);
  }
  return result;
}

function absoluteFrom(basePath: string, path: string): string {
  if (path.length === 0) {
    return resolvePath(basePath);
  }
  if (isAbsolutePath(path)) {
    return resolvePath(path);
  }
  return resolvePath(basePath, path);
}

function normalizeOptionalPath(basePath: string, path: string | undefined): string | undefined {
  if (path === undefined || path.length === 0) {
    return undefined;
  }
  return absoluteFrom(basePath, path);
}

function requiredPath(basePath: string, raw: RawObject, key: string): string {
  let value: string = getString(raw, key);
  if (value.length === 0) {
    throw driverError(ErrorCode.BUILDSYSTEM_INVALID_CONFIG, 'Malformed build config.', key + ' is required.');
  }
  return absoluteFrom(basePath, value);
}

function parseDependencyModule(raw: RawObject, basePath: string): DependencyModuleConfig {
  let packageName: string = getString(raw, 'packageName');
  let modulePathRaw: string = getString(raw, 'modulePath');
  if (packageName.length === 0 || modulePathRaw.length === 0) {
    throw driverError(ErrorCode.BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_CORRECT_FAIL, 'Dependent module info is not correct.', 'packageName and modulePath are required.');
  }
  let modulePath: string = absoluteFrom(basePath, modulePathRaw);
  ensurePathExists(modulePath, 'missing-module-path');
  let result: DependencyModuleConfig = {
    packageName: packageName,
    moduleName: getString(raw, 'moduleName', packageName),
    moduleType: getString(raw, 'moduleType', OHOS_MODULE_TYPE.HAR),
    modulePath: modulePath,
    sourceRoots: getStringArray(raw, 'sourceRoots'),
    entryFile: getString(raw, 'entryFile'),
    language: getString(raw, 'language', LANGUAGE_VERSION.ARKTS_1_2),
  };
  let dependencies: string[] = getStringArray(raw, 'dependencies');
  if (dependencies.length > 0) {
    result.dependencies = dependencies;
  }
  result.declFilesPath = normalizeOptionalPath(modulePath, optionalString(raw, 'declFilesPath'));
  result.abcPath = normalizeOptionalPath(modulePath, optionalString(raw, 'abcPath'));
  result.declgenV1OutPath = normalizeOptionalPath(modulePath, optionalString(raw, 'declgenV1OutPath'));
  result.declgenV2OutPath = normalizeOptionalPath(modulePath, optionalString(raw, 'declgenV2OutPath'));
  result.declgenBridgeCodePath = normalizeOptionalPath(modulePath, optionalString(raw, 'declgenBridgeCodePath'));
  result.byteCodeHar = getBoolean(raw, 'byteCodeHar', false);
  result.bundleType = optionalString(raw, 'bundleType');
  result.bundleName = optionalString(raw, 'bundleName');
  result.packageVersion = optionalString(raw, 'packageVersion');
  result.originalPackageNameMap = mapObjectToStringMap(hasKey(raw, 'originalPackageNameMap') ? raw['originalPackageNameMap'] : undefined);
  return result;
}

function parseDependencyModules(raw: RawObject, basePath: string): DependencyModuleConfig[] {
  if (!hasKey(raw, 'dependencyModuleList')) {
    return [];
  }
  let values: Object[] = raw['dependencyModuleList'] as Object[];
  let result: DependencyModuleConfig[] = [];
  for (let i: int = 0; i < values.length; i++) {
    result.push(parseDependencyModule(values[i] as RawObject, basePath));
  }
  return result;
}

function normalizeStringArrayPaths(basePath: string, values: string[]): string[] {
  let result: string[] = [];
  for (let i: int = 0; i < values.length; i++) {
    result.push(absoluteFrom(basePath, values[i]));
  }
  return result;
}

function parseBuildConfig(raw: RawObject, configPath: string): BuildConfig {
  let configDir: string = dirname(configPath);
  let moduleRootPath: string = requiredPath(configDir, raw, 'moduleRootPath');
  let buildSdkPath: string = requiredPath(configDir, raw, 'buildSdkPath');
  let pandaSdkPath: string = normalizeOptionalPath(configDir, optionalString(raw, 'pandaSdkPath')) ?? resolvePath(buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
  let buildConfig: BuildConfig = {
    packageName: getString(raw, 'packageName'),
    moduleType: getString(raw, 'moduleType') as OHOS_MODULE_TYPE,
    buildType: getString(raw, 'buildType', BUILD_TYPE.BUILD) as BUILD_TYPE,
    buildMode: getString(raw, 'buildMode', BUILD_MODE.DEBUG) as BUILD_MODE,
    es2pandaMode: getString(raw, 'es2pandaMode', ES2PANDA_MODE.RUN) as ES2PANDA_MODE,
    hasMainModule: getBoolean(raw, 'hasMainModule', true),
    isBuildConfigModified: false,
    recordType: getString(raw, 'recordType', RECORD_TYPE.DEFAULT_TYPE) as RECORD_TYPE,
    dumpDependencyGraph: getBoolean(raw, 'dumpDependencyGraph', false),
    dumpPerf: getBoolean(raw, 'dumpPerf', false),
    transformLib: optionalString(raw, 'transformLib'),
    moduleRootPath: moduleRootPath,
    sourceRoots: getStringArray(raw, 'sourceRoots'),
    byteCodeHar: getBoolean(raw, 'byteCodeHar', false),
    entryFile: getString(raw, 'entryFile'),
    bundleType: optionalString(raw, 'bundleType'),
    bundleName: optionalString(raw, 'bundleName'),
    loaderOutPath: normalizeOptionalPath(configDir, optionalString(raw, 'loaderOutPath')),
    cachePath: normalizeOptionalPath(configDir, optionalString(raw, 'cachePath')),
    buildSdkPath: buildSdkPath,
    pandaSdkPath: pandaSdkPath,
    pandaStdlibPath: normalizeOptionalPath(configDir, optionalString(raw, 'pandaStdlibPath')) ?? resolvePath(pandaSdkPath, PANDA_STDLIB_PATH_FROM_SDK),
    externalApiPaths: normalizeStringArrayPaths(configDir, getStringArray(raw, 'externalApiPaths')),
    sdkAliasConfigPaths: normalizeStringArrayPaths(configDir, getStringArray(raw, 'sdkAliasConfigPaths')),
    sdkAliasMap: mapObjectToStringMap(hasKey(raw, 'sdkAliasMap') ? raw['sdkAliasMap'] : undefined),
    interopApiPaths: normalizeStringArrayPaths(configDir, getStringArray(raw, 'interopApiPaths')),
    projectRootPath: normalizeOptionalPath(configDir, optionalString(raw, 'projectRootPath')) ?? configDir,
    arkGuardPath: normalizeOptionalPath(configDir, optionalString(raw, 'arkGuardPath')),
    toolChainsPath: normalizeOptionalPath(configDir, optionalString(raw, 'toolChainsPath')),
    disAsmStaticPath: normalizeOptionalPath(configDir, optionalString(raw, 'disAsmStaticPath')),
    aceModuleJsonPath: normalizeOptionalPath(configDir, optionalString(raw, 'aceModuleJsonPath')),
    aceProfilePath: normalizeOptionalPath(configDir, optionalString(raw, 'aceProfilePath')),
    aceModuleRoot: normalizeOptionalPath(configDir, optionalString(raw, 'aceModuleRoot')),
    frameworkMode: getBoolean(raw, 'frameworkMode', false),
    useEmptyPackage: getBoolean(raw, 'useEmptyPackage', false),
    enableDeclgenEts2Ts: getBoolean(raw, 'enableDeclgenEts2Ts', false),
    declgenV1OutPath: normalizeOptionalPath(configDir, optionalString(raw, 'declgenV1OutPath')),
    declgenV2OutPath: normalizeOptionalPath(configDir, optionalString(raw, 'declgenV2OutPath')),
    declgenBridgeCodePath: normalizeOptionalPath(configDir, optionalString(raw, 'declgenBridgeCodePath')),
    skipDeclCheck: getBoolean(raw, 'skipDeclCheck', false),
    continueOnError: getBoolean(raw, 'continueOnError', false),
    genDeclAnnotations: getBoolean(raw, 'genDeclAnnotations', false),
    enableDebugOutput: getBoolean(raw, 'enableDebugOutput', false),
    plugins: mapObjectToStringMap(hasKey(raw, 'plugins') ? raw['plugins'] : undefined),
    paths: mapObjectToStringArrayMap(hasKey(raw, 'paths') ? raw['paths'] : undefined),
    compileFiles: normalizeStringArrayPaths(configDir, getStringArray(raw, 'compileFiles')),
    dependencyModuleList: parseDependencyModules(raw, configDir),
  };
  buildConfig.nativeLibraryPath = resolveNativeCompilerLibraryPath();
  return buildConfig;
}

function checkRequiredSdkStubs(buildConfig: BuildConfig): void {
  let buildSdkPath: string = buildConfig.buildSdkPath as string;
  let requiredPaths: string[] = [
    resolvePath(buildSdkPath, API),
    resolvePath(buildSdkPath, ARKTS),
    resolvePath(buildSdkPath, KITS),
  ];
  for (let i: int = 0; i < requiredPaths.length; i++) {
    ensurePathExists(requiredPaths[i], 'missing-sdk-stub-path');
  }
}

function initAliasConfig(buildConfig: BuildConfig): void {
  let aliasConfig: Map<string, Map<string, AliasConfig>> = new Map<string, Map<string, AliasConfig>>();
  let sdkAliasMap: Map<string, string> = buildConfig.sdkAliasMap ?? new Map<string, string>();
  sdkAliasMap.forEach((filePath: string, packageName: string) => {
    let resolvedFilePath: string = resolvePath(filePath);
    ensureFileExists(resolvedFilePath, 'missing-alias-config');
    let content: string = readTextFile(resolvedFilePath);
    let rawAlias: RawObject = JSON.parse(content) as RawObject;
    let pkgAliases: Map<string, AliasConfig> = new Map<string, AliasConfig>();
    let keys: string[] = Object.keys(rawAlias);
    for (let i: int = 0; i < keys.length; i++) {
      let aliasKey: string = keys[i];
      let value: RawObject = rawAlias[aliasKey] as RawObject;
      if (!hasKey(value, 'originalAPIName') || !hasKey(value, 'isStatic')) {
        throw driverError(ErrorCode.BUILDSYSTEM_INIT_ALIAS_CONFIG_FAILED, 'Init Alias Config Failed', 'Invalid AliasConfig format in ' + packageName + ' -> ' + aliasKey);
      }
      pkgAliases.set(aliasKey, {
        originalAPIName: getString(value, 'originalAPIName'),
        isStatic: getBoolean(value, 'isStatic'),
      });
    }
    aliasConfig.set(packageName, pkgAliases);
    sdkAliasMap.set(packageName, resolvedFilePath);
  });
  buildConfig.sdkAliasMap = sdkAliasMap;
  buildConfig.aliasConfig = aliasConfig;
}

function initInteropSDKInfo(buildConfig: BuildConfig): void {
  let interopSDKPaths: Set<string> = new Set<string>();
  let basePaths: string[] = buildConfig.interopApiPaths !== undefined && (buildConfig.interopApiPaths as string[]).length > 0 ? buildConfig.interopApiPaths as string[] : [resolvePath(buildConfig.buildSdkPath as string, '../dynamic/build-tools/interop')];
  for (let i: int = 0; i < basePaths.length; i++) {
    let basePath: string = resolvePath(basePaths[i]);
    let arktsPath: string = resolvePath(basePath, ARKTS);
    let apiPath: string = resolvePath(basePath, API);
    let kitsPath: string = resolvePath(basePath, KITS);
    let componentPath: string = resolvePath(basePath, COMPONENT);
    if (fileExists(arktsPath)) {
      interopSDKPaths.add(arktsPath);
    }
    if (fileExists(apiPath)) {
      interopSDKPaths.add(apiPath);
    }
    if (fileExists(kitsPath)) {
      interopSDKPaths.add(kitsPath);
    }
    if (fileExists(componentPath)) {
      interopSDKPaths.add(componentPath);
    }
  }
  buildConfig.interopSDKPaths = interopSDKPaths;
}

export function initBuildConfig(buildConfigPath: string): BuildConfig {
  let resolvedConfigPath: string = resolvePath(buildConfigPath);
  ensureFileExists(resolvedConfigPath, 'missing-build-config');
  let rawText: string = readTextFile(resolvedConfigPath);
  let substitutedText: string = substituteEnvVarsInJSON(rawText);
  let substituted: Object = JSON.parse(substitutedText) as Object;
  let buildConfig: BuildConfig = parseBuildConfig(substituted as RawObject, resolvedConfigPath);
  checkRequiredSdkStubs(buildConfig);
  initAliasConfig(buildConfig);
  initInteropSDKInfo(buildConfig);
  return buildConfig;
}
