/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
import * as path from 'node:path';
import * as fs from 'fs';
import * as ts from 'typescript';
import { EXTNAME_JS, EXTNAME_TS, EXTNAME_D_ETS, EXTNAME_D_TS, EXTNAME_ETS } from '../utils/consts/ExtensionName';

interface ResolutionContext {
  sdkContext: SdkContext;
  projectPath: string;
  compilerOptions: ts.CompilerOptions;
}

interface SdkContext {
  allSDKPath: string[];
  systemModules: string[];
  sdkConfigPrefix: string;
  sdkDefaultApiPath: string;
}

export function readDeclareFiles(SdkPath: string): string[] {
  if (SdkPath === '') {
    return [];
  }
  const declarationsFileNames: string[] = [];
  const commonPath = path.resolve(SdkPath, './component/common.d.ts');
  if (fs.existsSync(commonPath)) {
    declarationsFileNames.push(path.resolve(SdkPath, './component/common.d.ts'));
  }
  const declarationsPath = path.resolve(SdkPath, './build-tools/ets-loader/declarations');
  if (!fs.existsSync(declarationsPath)) {
    throw new Error('get wrong sdkDefaultApiPath, declarationsPath not found');
  }
  fs.readdirSync(declarationsPath).forEach((fileName: string) => {
    if ((/\.d\.ts$/).test(fileName)) {
      declarationsFileNames.push(path.resolve(SdkPath, './build-tools/ets-loader/declarations', fileName));
    }
  });
  return declarationsFileNames;
}

export function createCompilerHost(
  sdkDefaultApiPath: string,
  sdkExternalApiPath: string[],
  arktsWholeProjectPath: string
): ts.CompilerHost {
  const sdkContext: SdkContext = setSdkContext(sdkDefaultApiPath, sdkExternalApiPath);
  const resolutionContext = createResolutionContext(sdkContext, arktsWholeProjectPath);
  const customCompilerHost: ts.CompilerHost = {
    getSourceFile(fileName: string, languageVersionOrOptions: ts.ScriptTarget): ts.SourceFile {
      return ts.createSourceFile(
        fileName,
        this.readFile(fileName) || '',
        languageVersionOrOptions,
        true,
        ts.ScriptKind.Unknown
      );
    },
    getDefaultLibFileName: (option: ts.CompilerOptions) => {
      return ts.getDefaultLibFilePath(option);
    },
    writeFile: ts.sys.writeFile,
    getCurrentDirectory: ts.sys.getCurrentDirectory,
    getCanonicalFileName: (fileName: string) => {
      return fileName;
    },
    useCaseSensitiveFileNames: () => {
      return ts.sys.useCaseSensitiveFileNames;
    },
    fileExists: ts.sys.fileExists,
    readFile: ts.sys.readFile,
    readDirectory: ts.sys.readDirectory,
    getNewLine: () => {
      return ts.sys.newLine;
    },
    resolveModuleNames: createModuleResolver(resolutionContext)
  };
  return customCompilerHost;
}

function getResolveModule(modulePath: string, type: string): ts.ResolvedModuleFull {
  return {
    resolvedFileName: modulePath,
    isExternalLibraryImport: false,
    extension: ts.Extension[type as keyof typeof ts.Extension]
  };
}

const fileExistsCache: Map<string, boolean> = new Map<string, boolean>();
const dirExistsCache: Map<string, boolean> = new Map<string, boolean>();
const moduleResolutionHost: ts.ModuleResolutionHost = {
  fileExists: (fileName: string): boolean => {
    let exists = fileExistsCache.get(fileName);
    if (exists === undefined) {
      exists = ts.sys.fileExists(fileName);
      fileExistsCache.set(fileName, exists);
    }
    return exists;
  },
  directoryExists: (directoryName: string): boolean => {
    let exists = dirExistsCache.get(directoryName);
    if (exists === undefined) {
      exists = ts.sys.directoryExists(directoryName);
      dirExistsCache.set(directoryName, exists);
    }
    return exists;
  },
  readFile(fileName: string): string | undefined {
    return ts.sys.readFile(fileName);
  },
  realpath(path: string): string {
    if (ts.sys.realpath) {
      return ts.sys.realpath(path);
    }
    return path;
  },
  trace(s: string): void {
    console.info(s);
  }
};

export interface ResolveModuleInfo {
  modulePath: string;
  isEts: boolean;
}

export function getRealModulePath(apiDirs: string, moduleName: string, exts: string[]): ResolveModuleInfo {
  const resolveResult: ResolveModuleInfo = {
    modulePath: '',
    isEts: true
  };
  const dir = apiDirs;
  for (let i = 0; i < exts.length; i++) {
    const ext = exts[i];
    const moduleDir = path.resolve(dir, moduleName + ext);
    if (!fs.existsSync(moduleDir)) {
      continue;
    }
    resolveResult.modulePath = moduleDir;
    if (ext === EXTNAME_D_TS) {
      resolveResult.isEts = false;
    }
    break;
  }

  return resolveResult;
}

export const shouldResolvedFiles: Set<string> = new Set();
export const resolvedModulesCache: Map<string, ts.ResolvedModuleFull[]> = new Map();
function createModuleResolver(
  context: ResolutionContext
): (moduleNames: string[], containingFile: string) => ts.ResolvedModuleFull[] {
  return (moduleNames: string[], containingFile: string): ts.ResolvedModuleFull[] => {
    return resolveModules(moduleNames, containingFile, context);
  };
}

function resolveModules(
  moduleNames: string[],
  containingFile: string,
  context: ResolutionContext
): ts.ResolvedModuleFull[] {
  const resolvedModules: ts.ResolvedModuleFull[] = [];
  const cacheKey = path.resolve(containingFile);
  const cacheFileContent = resolvedModulesCache.get(cacheKey);

  if (shouldResolveModules(moduleNames, containingFile, cacheFileContent)) {
    for (const moduleName of moduleNames) {
      const resolvedModule = resolveModule(moduleName, containingFile, context);
      // @ts-expect-error null should push
      resolvedModules.push(resolvedModule);
    }
    resolvedModulesCache.set(cacheKey, resolvedModules);
  } else {
    resolvedModulesCache.delete(cacheKey);
  }

  return resolvedModules;
}

function shouldResolveModules(
  moduleNames: string[],
  containingFile: string,
  cacheFileContent: ts.ResolvedModuleFull[] | undefined
): boolean {
  const resolvedFilePath = path.resolve(containingFile);
  const isCacheValid = cacheFileContent && cacheFileContent.length === moduleNames.length;
  return ![...shouldResolvedFiles].length || shouldResolvedFiles.has(resolvedFilePath) || !isCacheValid;
}

function resolveModule(
  moduleName: string,
  containingFile: string,
  context: ResolutionContext
): ts.ResolvedModuleFull | null {
  const result = ts.resolveModuleName(moduleName, containingFile, context.compilerOptions, moduleResolutionHost);
  if (result.resolvedModule) {
    return handleResolvedModule(result.resolvedModule);
  }

  if (isSdkModule(moduleName, context)) {
    return resolveSdkModule(moduleName, context);
  }

  if (isEtsModule(moduleName)) {
    return resolveEtsModule(moduleName, containingFile);
  }

  if (isTsModule(moduleName)) {
    return resolveTsModule(moduleName, containingFile);
  }

  return resolveDefaultModule(moduleName, containingFile, context);
}

function handleResolvedModule(resolvedModule: ts.ResolvedModuleFull): ts.ResolvedModuleFull {
  if (resolvedModule.resolvedFileName && path.extname(resolvedModule.resolvedFileName) === EXTNAME_JS) {
    const detsPath = resolvedModule.resolvedFileName.replace(EXTNAME_JS, EXTNAME_D_ETS);
    return ts.sys.fileExists(detsPath) ? getResolveModule(detsPath, EXTNAME_D_ETS) : resolvedModule;
  }
  return resolvedModule;
}

function isSdkModule(moduleName: string, context: ResolutionContext): boolean {
  return new RegExp(`^@(${context.sdkContext.sdkConfigPrefix})\\.`, 'i').test(moduleName.trim());
}

function resolveSdkModule(moduleName: string, context: ResolutionContext): ts.ResolvedModuleFull | null {
  for (const sdkPath of context.sdkContext.allSDKPath) {
    const resolveModuleInfo = getRealModulePath(sdkPath, moduleName, [EXTNAME_D_TS, EXTNAME_D_ETS]);
    const modulePath = resolveModuleInfo.modulePath;
    const isDETS = resolveModuleInfo.isEts;

    if (
      context.sdkContext.systemModules.includes(moduleName + (isDETS ? EXTNAME_D_ETS : EXTNAME_D_TS)) &&
      ts.sys.fileExists(modulePath)
    ) {
      return getResolveModule(modulePath, isDETS ? EXTNAME_D_ETS : EXTNAME_D_TS);
    }
  }
  return null;
}

function isEtsModule(moduleName: string): boolean {
  return (/\.ets$/).test(moduleName) && !(/\.d\.ets$/).test(moduleName);
}

function resolveEtsModule(moduleName: string, containingFile: string): ts.ResolvedModuleFull | null {
  const modulePath = path.resolve(path.dirname(containingFile), moduleName);
  return ts.sys.fileExists(modulePath) ? getResolveModule(modulePath, EXTNAME_ETS) : null;
}

function isTsModule(moduleName: string): boolean {
  return (/\.ts$/).test(moduleName);
}

function resolveTsModule(moduleName: string, containingFile: string): ts.ResolvedModuleFull | null {
  const modulePath = path.resolve(path.dirname(containingFile), moduleName);
  return ts.sys.fileExists(modulePath) ? getResolveModule(modulePath, EXTNAME_TS) : null;
}

function resolveDefaultModule(
  moduleName: string,
  containingFile: string,
  context: ResolutionContext
): ts.ResolvedModuleFull | null {
  const { sdkContext } = context;
  const { sdkDefaultApiPath } = sdkContext;

  const paths = [
    path.resolve(sdkDefaultApiPath, './api', moduleName + EXTNAME_D_TS),
    path.resolve(sdkDefaultApiPath, './api', moduleName + EXTNAME_D_ETS),
    path.resolve(sdkDefaultApiPath, './kits', moduleName + EXTNAME_D_TS),
    path.resolve(sdkDefaultApiPath, './kits', moduleName + EXTNAME_D_ETS),
    path.resolve(
      sdkDefaultApiPath,
      './ets_loader/node_modules',
      moduleName + ((/\./).test(moduleName) ? '' : EXTNAME_JS)
    ),
    path.resolve(sdkDefaultApiPath, './ets_loader/node_modules', moduleName + '/index.js'),
    path.resolve(path.dirname(containingFile), (/\.d\.ets$/).test(moduleName) ? moduleName : moduleName + EXTNAME_D_ETS)
  ];

  for (const filePath of paths) {
    if (ts.sys.fileExists(filePath)) {
      const ext = path.extname(filePath);
      return getResolveModule(filePath, ext);
    }
  }

  const srcIndex = containingFile.indexOf('src' + path.sep + 'main');
  if (srcIndex > 0) {
    const detsModulePathFromModule = path.resolve(
      containingFile.substring(0, srcIndex),
      moduleName + path.sep + 'index' + EXTNAME_D_ETS
    );
    return ts.sys.fileExists(detsModulePathFromModule) ?
      getResolveModule(detsModulePathFromModule, EXTNAME_D_ETS) :
      null;
  }

  return null;
}

function createResolutionContext(sdkContext: SdkContext, projectPath: string): ResolutionContext {
  return {
    sdkContext,
    projectPath,
    compilerOptions: createCompilerOptions(sdkContext, projectPath)
  };
}

function createCompilerOptions(sdkContext: SdkContext, projectPath: string): ts.CompilerOptions {
  const compilerOptions: ts.CompilerOptions = ((): ts.CompilerOptions => {
    const configPath = path.resolve(sdkContext.sdkDefaultApiPath, './build-tools/ets-loader/tsconfig.json');
    if (!fs.existsSync(configPath)) {
      throw new Error('get wrong sdkDefaultApiPath, tsconfig.json not found');
    }
    const configFile = ts.readConfigFile(configPath, ts.sys.readFile);
    return configFile.config.compilerOptions;
  })();
  Object.assign(compilerOptions, {
    allowJs: true,
    checkJs: false,
    emitNodeModulesFiles: true,
    importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Preserve,
    module: ts.ModuleKind.CommonJS,
    moduleResolution: ts.ModuleResolutionKind.NodeJs,
    noEmit: true,
    baseUrl: projectPath,
    packageManagerType: 'ohpm'
  });
  return compilerOptions;
}
function createInitialContext(sdkDefaultApiPath: string): SdkContext {
  return {
    allSDKPath: [],
    systemModules: [],
    sdkConfigPrefix: 'ohos|system|kit|arkts',
    sdkDefaultApiPath
  };
}

function processBasePath(basePath: string): string[] {
  if (!fs.existsSync(basePath)) {
    return [];
  }
  return fs.readdirSync(basePath).filter((name) => {
    return !name.startsWith('.');
  });
}

function getBasePaths(sdkDefaultApiPath: string): string[] {
  return [
    path.resolve(sdkDefaultApiPath, './api'),
    path.resolve(sdkDefaultApiPath, './arkts'),
    path.resolve(sdkDefaultApiPath, './kits')
  ];
}

function processExternalConfig(externalPath: string): { modules: string[]; paths: string[]; prefix?: string } | null {
  const configPath = path.resolve(externalPath, 'sdkConfig.json');
  if (!fs.existsSync(configPath)) {
    return null;
  }

  try {
    const config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));
    if (!config.apiPath) {
      return null;
    }

    const result = {
      modules: [] as string[],
      paths: [] as string[],
      prefix: config.prefix?.replace(/^@/, '')
    };

    config.apiPath.forEach((relPath: string) => {
      const absPath = path.resolve(externalPath, relPath);
      if (fs.existsSync(absPath)) {
        result.modules.push(...processBasePath(absPath));
        result.paths.push(absPath);
      }
    });

    return result;
  } catch (e) {
    console.error(`Error processing SDK config: ${configPath}`, e);
    return null;
  }
}

function processExternalPaths(externalPaths: string[]): { modules: string[]; paths: string[]; prefixes: string[] } {
  const result = {
    modules: [] as string[],
    paths: [] as string[],
    prefixes: [] as string[]
  };

  externalPaths.forEach((externalPath) => {
    const configResult = processExternalConfig(externalPath);
    if (!configResult) {
      return;
    }

    result.modules.push(...configResult.modules);
    result.paths.push(...configResult.paths);
    if (configResult.prefix) {
      result.prefixes.push(configResult.prefix);
    }
  });

  return result;
}

export function setSdkContext(sdkDefaultApiPath: string, sdkExternalApiPath: string[]): SdkContext {
  const context = createInitialContext(sdkDefaultApiPath);

  // Process base SDK paths
  const basePaths = getBasePaths(sdkDefaultApiPath);
  basePaths.forEach((p) => {
    context.systemModules.push(...processBasePath(p));
  });
  context.allSDKPath.push(...basePaths);

  // Process external SDK paths
  const externalResults = processExternalPaths(sdkExternalApiPath);
  context.systemModules.push(...externalResults.modules);
  context.allSDKPath.push(...externalResults.paths);
  if (externalResults.prefixes.length > 0) {
    context.sdkConfigPrefix += `|${externalResults.prefixes.join('|')}`;
  }

  return context;
}
