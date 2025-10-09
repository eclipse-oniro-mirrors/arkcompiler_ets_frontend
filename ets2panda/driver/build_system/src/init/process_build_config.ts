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

import * as fs from 'fs';
import * as path from 'path';

import {
  isLinux,
  isMac,
  isWindows,
} from '../util/utils';
import { PluginDriver } from '../plugins/plugins_driver';
import {
  API,
  ARKTS,
  COMPONENT,
  KITS,
  PANDA_SDK_PATH_FROM_SDK,
  PROJECT_BUILD_CONFIG_FILE
} from '../pre_define';
import {
  LogData,
  LogDataFactory,
  Logger
} from '../logger';
import { ErrorCode } from '../error_code';
import {
  BuildConfig,
  BUILD_MODE,
  AliasConfig
} from '../types';
import { initKoalaModules } from './init_koala_modules';

export function processBuildConfig(projectConfig: BuildConfig): BuildConfig {
  let buildConfig: BuildConfig = { 
    ...projectConfig,
    isBuildConfigModified: false 
  };
  let buildSdkPath: string = buildConfig.buildSdkPath as string;
  buildConfig.pandaSdkPath = buildConfig.pandaSdkPath ?? path.resolve(buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
  /**
   * ets2panda guys require remove debug param
   * it contain some bugs.
   */
  buildConfig.buildMode = BUILD_MODE.RELEASE;
  checkCacheProjectConfig(buildConfig);
  initPlatformSpecificConfig(buildConfig);
  initBuildEnv(buildConfig);
  initKoalaModules(buildConfig);
  PluginDriver.getInstance().initPlugins(buildConfig);
  initAliasConfig(buildConfig);
  initInteropSDKInfo(buildConfig);
  return buildConfig;
}

function checkCacheProjectConfig(buildConfig: BuildConfig): void {
  const cachePath = buildConfig.cachePath as string;
  const projectionConfigPath = path.join(cachePath, PROJECT_BUILD_CONFIG_FILE);
  const logger: Logger = Logger.getInstance();

  if (!fs.existsSync(cachePath)) {
    fs.mkdirSync(cachePath, { recursive: true });
  }

  if (!fs.existsSync(projectionConfigPath)) {
    fs.writeFileSync(projectionConfigPath, JSON.stringify(buildConfig, null, '\t'));
  } else {
    const existingConfig = JSON.parse(fs.readFileSync(projectionConfigPath, 'utf8'));
    if (!areConfigsEqual(existingConfig, buildConfig)) {
      buildConfig.isBuildConfigModified = true;
      fs.writeFileSync(projectionConfigPath, JSON.stringify(buildConfig, null, '\t'));
    } else {
      buildConfig.isBuildConfigModified = false;
      logger.printInfo('projectionConfig.json is up to date.');
    }
  }
}

function areConfigsEqual(config1: BuildConfig, config2: BuildConfig): boolean {
  const { isBuildConfigModified: _, compileFiles: __, ...rest1 } = config1;
  const { isBuildConfigModified: ___, compileFiles: ____, ...rest2 } = config2;
  return JSON.stringify(rest1) === JSON.stringify(rest2);
}

function initPlatformSpecificConfig(buildConfig: BuildConfig): void {
  const pandaSdkPath: string = path.resolve(buildConfig.pandaSdkPath as string);
  const logger: Logger = Logger.getInstance();
  if (isWindows()) {
    buildConfig.abcLinkerPath = path.join(pandaSdkPath, 'bin', 'ark_link.exe');
    buildConfig.dependencyAnalyzerPath = path.join(pandaSdkPath, 'bin', 'dependency_analyzer.exe');
  }

  if (isMac() || isLinux()) {
    buildConfig.abcLinkerPath = path.join(pandaSdkPath, 'bin', 'ark_link');
    buildConfig.dependencyAnalyzerPath = path.join(pandaSdkPath, 'bin', 'dependency_analyzer');
  }

  if (!buildConfig.enableDeclgenEts2Ts && !fs.existsSync(buildConfig.abcLinkerPath as string)) {
    const logData: LogData = LogDataFactory.newInstance(
      ErrorCode.BUILDSYSTEM_ARK_LINK_NOT_FOUND_FAIL,
      'Ark_link not found in path.',
      '',
      buildConfig.abcLinkerPath as string
    );
    logger.printError(logData);
  }

  if (!buildConfig.frameworkMode && !buildConfig.enableDeclgenEts2Ts && !fs.existsSync(buildConfig.dependencyAnalyzerPath as string)) {
    const logData: LogData = LogDataFactory.newInstance(
      ErrorCode.BUILDSYSTEM_Dependency_Analyzer_NOT_FOUND_FAIL,
      'Dependency_analyzer not found in path.',
      '',
      buildConfig.dependencyAnalyzerPath as string
    );
    logger.printError(logData);
  }
}

export function initBuildEnv(buildConfig: BuildConfig): void {
  const pandaSdkPath: string = path.resolve(buildConfig.pandaSdkPath as string);
  const currentPath: string | undefined = process.env.PATH;
  const logger: Logger = Logger.getInstance();

  let pandaLibPath: string = path.resolve(pandaSdkPath, 'lib');

  process.env.PATH = `${currentPath}${path.delimiter}${pandaLibPath}`;
  if (isMac()) {
    process.env.DYLD_LIBRARY_PATH = `${currentPath}${path.delimiter}${pandaLibPath}`;
  }
  logger.printInfo(`Updated PATH: ${process.env.PATH}`);
}

function initAliasConfig(buildConfig: BuildConfig): void {
  buildConfig.aliasConfig = {};
  buildConfig.sdkAliasMap = buildConfig.sdkAliasMap instanceof Map
    ? buildConfig.sdkAliasMap
    : new Map(Object.entries(buildConfig.sdkAliasMap || {}));

  if (buildConfig.sdkAliasMap.size === 0) {
    return;
  }
  for (const [pkgName, filePath] of buildConfig.sdkAliasMap) {
    const rawContent = fs.readFileSync(filePath, 'utf-8');
    const jsonData = JSON.parse(rawContent);
    const pkgAliasObj: Record<string, AliasConfig> = {};

    for (const [aliasKey, config] of Object.entries(jsonData)) {
      if (typeof config !== 'object' || config === null ||
        !('originalAPIName' in config) || !('isStatic' in config)) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_INIT_ALIAS_CONFIG_FAILED,
          'Init Alias Config Failed',
          `Invalid AliasConfig format in ${pkgName} -> ${aliasKey}`
        );
        Logger.getInstance().printErrorAndExit(logData);
      }

      const aliasConfig = config as AliasConfig;
      pkgAliasObj[aliasKey] = {
        originalAPIName: aliasConfig.originalAPIName,
        isStatic: aliasConfig.isStatic
      };
    }

    buildConfig.aliasConfig[pkgName] = pkgAliasObj;
  }
}

function initInteropSDKInfo(buildConfig: BuildConfig): void {
  buildConfig.interopSDKPaths = new Set<string>();

  const basePaths = buildConfig.interopApiPaths?.length
    ? buildConfig.interopApiPaths
    : [path.resolve(buildConfig.buildSdkPath as string, '../ets1.1/build-tools/interop')];

  for (const basePath of basePaths) {
    /**
     * dynamic public api from 1.1
     */
    const arktsPath = path.resolve(basePath, ARKTS);
    /**
     * dynamic public api from 1.1
     */
    const apiPath = path.resolve(basePath, API);
    /**
     * a router file from 1.1, whicl will export * from manay api file
     * and kit have not runtime_name,is alias for edit,
     * and will be transformed before compile in 1.1
     */
    const kitsPath = path.resolve(basePath, KITS);
    /**
     * component is inner api for apiPath and artsPath
     * bcs apiPath and artsPath is dynamic module,
     * apiPath will depend component, we should also add component to dependenciesection,
     * or it will fatal error
     */
    const component = path.resolve(basePath, COMPONENT);


    if (fs.existsSync(arktsPath)) {
      buildConfig.interopSDKPaths.add(arktsPath);
    }
    if (fs.existsSync(apiPath)) {
      buildConfig.interopSDKPaths.add(apiPath);
    }
    if (fs.existsSync(kitsPath)) {
      buildConfig.interopSDKPaths.add(kitsPath);
    }
    if (fs.existsSync(component)) {
      buildConfig.interopSDKPaths.add(component);
    }
  }
}

export function getEs2pandaPath(buildConfig:BuildConfig):string{
  const pandaSdkPath = buildConfig.pandaSdkPath ?? path.resolve(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
  let  es2pandaPath = '';
  if (isWindows()) {
    es2pandaPath = path.join(pandaSdkPath, 'bin', 'es2panda.exe');
  }

  if (isMac() || isLinux()) {
    es2pandaPath = path.join(pandaSdkPath, 'bin', 'es2panda');
  }
  return es2pandaPath;
}