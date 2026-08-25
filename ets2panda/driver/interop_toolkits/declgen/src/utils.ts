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

import * as os from 'node:os';
import * as path from 'node:path';
import * as fs from 'node:fs';

import * as common from '@interop-toolkits/common';

import type { BuildConfig, ModuleConfig } from './buildConfig';
import ts from 'typescript';

export function generateArktsconfig(buildConfig: BuildConfig, cacheDir: string, outputPath: string): Promise<void> {
  if (buildConfig.pandaSdkPath === undefined) {
    throw new common.errors.InternalError('Panda SDK path is not defined in the build configuration');
  }
  const interopContexts = new Map(
    buildConfig.dependentModuleList.map((module) => [module.packageName, buildDynamicInteropContext(module)]),
  );
  const sourceContext = common.arktsconfig.createArkTSConfigSourceContext({
    projectRootPath: buildConfig.projectRootPath,
    cachePath: cacheDir,
    pandaSdkPath: buildConfig.pandaSdkPath,
    packageName: buildConfig.packageName,
    bundleName: buildConfig.bundleName,
    moduleType: buildConfig.moduleType,
    dependentModuleList: buildConfig.dependentModuleList.map(generateArktsconfigModule),
    interopContexts,
    externalApiPaths: buildConfig.externalApiPaths,
    dynamicInteropSdkPaths: buildConfig.sdkPaths.dynamicInteropSdkPaths,
  });
  return new common.arktsconfig.ArkTSConfigGenerator(sourceContext).write(outputPath);
}

export function getDepAnalyzerPath(pandaSdkPath: string): string {
  if (os.type() === 'Windows_NT') {
    return path.join(pandaSdkPath, 'bin', 'dependency_analyzer.exe');
  }
  if (os.type() === 'Darwin' || os.type() === 'Linux') {
    const buildDir = process.env.BUILD_DIR;
    return path.join(buildDir ?? pandaSdkPath, 'bin', 'dependency_analyzer');
  }
  throw new common.errors.InternalError(`Unsupported platform: ${os.type()}`);
}

export function getDynamicTsConfigPath(buildConfig: BuildConfig): string {
  return path.join(buildConfig.buildDynamicSdkPath, 'build-tools', 'ets-loader', 'tsconfig.json');
}

export function loadTsCompilerOptions(tsconfigPath: string): ts.CompilerOptions {
  const tsConfig = JSON.parse(fs.readFileSync(tsconfigPath, 'utf-8'));
  const compilerOptions = ts.convertCompilerOptionsFromJson(
    tsConfig.compilerOptions ?? {},
    path.dirname(tsconfigPath),
  ).options;
  Object.assign(compilerOptions, {
    emitNodeModulesFiles: true,
    importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Preserve,
    module: ts.ModuleKind.CommonJS,
    moduleResolution: ts.ModuleResolutionKind.NodeJs,
    noEmit: true,
    packageManagerType: 'ohpm',
    allowJs: true,
    allowSyntheticDefaultImports: true,
    esModuleInterop: true,
    noImplicitAny: false,
    noUnusedLocals: false,
    noUnusedParameters: false,
    experimentalDecorators: true,
    resolveJsonModule: true,
    skipLibCheck: false,
    sourceMap: true,
    target: 8,
    types: [],
    typeRoots: [],
    lib: ['lib.es2021.d.ts'],
    alwaysStrict: true,
    checkJs: false,
    maxFlowDepth: 2000,
    etsAnnotationsEnable: false,
    etsLoaderPath: path.dirname(tsconfigPath),
    needDoArkTsLinter: true,
    isCompatibleVersion: false,
    skipTscOhModuleCheck: false,
    skipArkTSStaticBlocksCheck: false,
    incremental: true,
    tsImportSendableEnable: false,
    skipPathsInKeyForCompilationSettings: true,
  });
  compilerOptions.ets!.customComponent = undefined;
  return compilerOptions;
}

function generateArktsconfigModule(module: ModuleConfig): common.arktsconfig.ModuleInput {
  return {
    packageName: module.packageName,
    moduleName: module.moduleName,
    moduleType: module.moduleType,
    modulePath: module.modulePath,
    sourceRoots: module.sourceRoots,
    entryFile: module.entryFile,
    language: module.language,
    dependencies: module.dependencies ?? [],
    abcPath: module.abcPath,
    bundleType: module.bundleType,
    bundleName: module.bundleName,
    packageVersion: module.packageVersion,
    originalPackageNameMap: module.originalPackageNameMap ?? {},
  };
}

function buildDynamicInteropContext(module: ModuleConfig): common.arktsconfig.DynamicInteropContext {
  if (module.declgenV2OutPath === undefined) {
    throw new common.errors.InternalError(
      `Module ${module.packageName} does not have a interop declaration output path`,
    );
  }
  const packageName = module.packageName;
  const declgenV2OutPath = module.declgenV2OutPath;
  const finalObj: Record<string, common.arktsconfig.DynamicInteropFile> = {};
  module.dynamicFiles.forEach((filePath) => {
    const projectFilePath = getRelativePath(filePath, module.modulePath);
    const moduleName = module.moduleName ?? packageName;
    const normalizedFilePath = module.isNative ? moduleName : `${packageName}/${projectFilePath}`;
    const declPath = path.join(toUnixPath(declgenV2OutPath), projectFilePath) + common.fileUtils.Extension.DETS;
    const isNativeFlag = module.isNative ? 'Y' : 'N';
    finalObj[projectFilePath] = {
      declPath,
      filePath: toUnixPath(filePath),
      ohmUrl: `@normalized:${isNativeFlag}&&&${normalizedFilePath}&`,
    };
  });

  return {
    packageName,
    files: finalObj,
  };
}

function getRelativePath(filePath: string, packagePath: string): string {
  const sourceFilePath = filePath.charCodeAt(0) === 0 ? filePath.slice(1) : filePath;
  const unixFilePath = toUnixPath(sourceFilePath);
  const sourcePath = unixFilePath.endsWith('.d.ets')
    ? unixFilePath.slice(0, -'.d.ets'.length)
    : unixFilePath.endsWith('.d.ts')
      ? unixFilePath.slice(0, -'.d.ts'.length)
      : unixFilePath.slice(0, unixFilePath.lastIndexOf('.'));
  return sourcePath.replace(`${toUnixPath(packagePath)}/`, '');
}

function toUnixPath(filePath: string): string {
  if (!/^win/.test(os.platform())) {
    return filePath;
  }
  return path.posix.join(...filePath.split(path.sep));
}

export function initBuildConfig(buildConfig: BuildConfig): void {
  if (!buildConfig.pandaSdkPath) {
    Object.assign(buildConfig, buildConfig, {
      pandaSdkPath: path.join(buildConfig.buildSdkPath, 'build-tools', 'ets2panda'),
    });
  }
}
