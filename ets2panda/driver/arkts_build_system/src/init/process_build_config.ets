/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import { global } from '@arkts-bindings/index';
import { API, ARKTS, COMPONENT, KITS, PANDA_SDK_PATH_FROM_SDK } from '../pre_define';
import { AliasConfig, BuildConfig } from '../types';
import { Logger, LogDataFactory } from '../logger';
import { ErrorCode } from '../util/error';

export function initBuildConfig(projectConfig: BuildConfig): BuildConfig {
  const buildConfig = projectConfig;
  buildConfig.isBuildConfigModified = false;
  buildConfig.pandaSdkPath = buildConfig.pandaSdkPath ?? resolvePath(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
  initPlatformSpecificConfig(buildConfig);
  initBuildEnv(buildConfig);
  initAliasConfig(buildConfig);
  initInteropSDKInfo(buildConfig);
  return buildConfig;
}

export function initBuildEnv(buildConfig: BuildConfig): void {
  const logger = Logger.getInstance();
  logger.printInfo('ArkTS build system uses native process environment; panda sdk path: ' + (buildConfig.pandaSdkPath ?? ''));
}

export function initPlatformSpecificConfig(buildConfig: BuildConfig): void {
  const pandaSdkPath = resolvePath(buildConfig.pandaSdkPath ?? resolvePath(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK));
  const abcLinkerPath = joinPath(pandaSdkPath, 'bin', 'ark_link');
  const dependencyAnalyzerPath = joinPath(pandaSdkPath, 'bin', 'dependency_analyzer');
  buildConfig.abcLinkerPath = abcLinkerPath;
  buildConfig.dependencyAnalyzerPath = dependencyAnalyzerPath;
  const logger = Logger.getInstance();
  if (!buildConfig.enableDeclgenEts2Ts && !global.interop._FileExists(abcLinkerPath)) {
    logger.printError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_ARK_LINK_NOT_FOUND_FAIL, 'Ark_link not found in path.', '', abcLinkerPath));
  }
  if (!buildConfig.frameworkMode && !buildConfig.enableDeclgenEts2Ts && !global.interop._FileExists(dependencyAnalyzerPath)) {
    logger.printError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZER_NOT_FOUND_FAIL, 'Dependency_analyzer not found in path.', '', dependencyAnalyzerPath));
  }
}

export function initAliasConfig(buildConfig: BuildConfig): void {
  buildConfig.aliasConfig = new Map<string, Map<string, AliasConfig>>();
  if (buildConfig.sdkAliasMap === undefined) {
    buildConfig.sdkAliasMap = new Map<string, string>();
  }
}

export function initInteropSDKInfo(buildConfig: BuildConfig): void {
  buildConfig.interopSDKPaths = new Set<string>();
  const basePaths = buildConfig.interopApiPaths.length > 0 ? buildConfig.interopApiPaths : [resolvePath(buildConfig.buildSdkPath, '../dynamic/build-tools/interop')];
  for (const basePath of basePaths) {
    addIfExists(buildConfig.interopSDKPaths, resolvePath(basePath, ARKTS));
    addIfExists(buildConfig.interopSDKPaths, resolvePath(basePath, API));
    addIfExists(buildConfig.interopSDKPaths, resolvePath(basePath, KITS));
    addIfExists(buildConfig.interopSDKPaths, resolvePath(basePath, COMPONENT));
  }
}

export function getEs2pandaPath(buildConfig: BuildConfig): string {
  const pandaSdkPath = buildConfig.pandaSdkPath ?? resolvePath(buildConfig.buildSdkPath, PANDA_SDK_PATH_FROM_SDK);
  return joinPath(pandaSdkPath, 'bin', 'es2panda');
}

export function readTextFile(path: string): string {
  return global.interop._ReadFile(path);
}

export function fileExists(path: string): boolean {
  return global.interop._FileExists(path);
}

export function joinPath(base: string, part1: string, part2: string = ''): string {
  let path = base.endsWith('/') ? base + part1 : base + '/' + part1;
  if (part2 !== '') {
    path = path.endsWith('/') ? path + part2 : path + '/' + part2;
  }
  return normalizePath(path);
}

export function resolvePath(base: string, part: string = ''): string {
  if (part === '') {
    return normalizePath(base);
  }
  if (part.startsWith('/')) {
    return normalizePath(part);
  }
  return normalizePath(joinPath(base, part));
}

export function normalizePath(path: string): string {
  const segments = path.split('/');
  const normalized: string[] = [];
  for (const segment of segments) {
    if (segment === '' || segment === '.') {
      continue;
    }
    if (segment === '..') {
      if (normalized.length > 0) {
        normalized.pop();
      }
      continue;
    }
    normalized.push(segment);
  }
  const prefix = path.startsWith('/') ? '/' : '';
  return prefix + normalized.join('/');
}

function addIfExists(set: Set<string>, path: string): void {
  if (global.interop._FileExists(path)) {
    set.add(path);
  }
}
