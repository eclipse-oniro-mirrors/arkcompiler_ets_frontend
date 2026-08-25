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

import * as fs from 'node:fs';
import * as path from 'node:path';
import * as common from '@interop-toolkits/common';
import { PluginDriver, PluginHook } from '../plugins';
import { loadLibArkts } from '../libarkts';
import type { KPointer, LibArkts } from '../libarkts';
import type { BuildConfig } from '../../buildConfig';

const ETSCACHE_SUFFIX = '.etscache';

export class Executor {
  private libarkts: LibArkts;
  private pluginDriver: PluginDriver;
  private projectRootPath: string;
  private fileManager: common.fileManager.FileManager;
  private cacheDir?: string;
  constructor(buildConfig: BuildConfig) {
    this.libarkts = loadLibArkts(buildConfig.pandaSdkPath!, buildConfig.buildSdkPath);
    this.fileManager = new common.fileManager.FileManagerBuilder()
      .addDynamicSdkPaths(buildConfig.sdkPaths.dynamicSdkPaths)
      .addStaticSdkPaths(buildConfig.sdkPaths.staticSdkPaths)
      .addDynamicInteropSdkPaths(buildConfig.sdkPaths.dynamicInteropSdkPaths)
      .addStaticInteropSdkPaths(buildConfig.sdkPaths.staticInteropSdkPaths)
      .addModuleList(buildConfig.dependentModuleList)
      .build();
    this.pluginDriver = new PluginDriver(this.fileManager);
    this.pluginDriver.initPlugins(buildConfig);
    this.projectRootPath = buildConfig.projectRootPath;
    this.cacheDir = buildConfig.cachePath;
  }

  execute(inputFiles: string[], arktsconfigPath: string): void {
    const { arkts, arktsGlobal } = this.libarkts;
    // Initialize the ArkTS memory management system
    arkts.memInitialize();
    const ets2pandaCmd = formDeclgenCliCmd(this.projectRootPath, arktsconfigPath);
    let declgen: KPointer | undefined;
    try {
      arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
      arktsGlobal.compilerContext = arkts.Context.createContextSimultaneousMode(inputFiles);
      const outputDeclEtsPaths: string[] = [];
      for (const file of inputFiles) {
        const moduleInfo = this.fileManager.queryFileMeta(file)?.module;
        if (moduleInfo === undefined) {
          throw new Error(`Cannot resolve module info for declgen input file: ${file}`);
        }
        const { declEtsOutputPath } = buildDeclgenOutputPath(file, moduleInfo, this.cacheDir);
        outputDeclEtsPaths.push(declEtsOutputPath);
      }
      const outputEtsPaths = inputFiles.map(() => '');
      this.pluginDriver.getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);
      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer, true);
      declgen = arkts.createTsDeclgen(inputFiles, outputDeclEtsPaths, outputEtsPaths, false, false, '', true);
      let ast = arkts.EtsScript.fromContext();
      this.pluginDriver.getPluginContext().setArkTSAst(ast);
      this.pluginDriver.runPluginHook(PluginHook.PARSED);
      arkts.generateTsDeclarationsAfterParsed(declgen);

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer, true);
      ast = arkts.EtsScript.fromContext();
      this.pluginDriver.getPluginContext().setArkTSAst(ast);
      this.pluginDriver.runPluginHook(PluginHook.CHECKED);
      arkts.generateTsDeclarationsAfterCheck(declgen);

      arkts.writeTsDeclarations(declgen);
    } catch (error) {
      throw Error(`Declgen failed: ${error instanceof Error ? error.message : String(error)}`);
    } finally {
      this.pluginDriver.runPluginHook(PluginHook.CLEAN);
      if (declgen) {
        arkts.destroyTsDeclgen(declgen);
      }
      if (arktsGlobal.compilerContext) {
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
      }
      arkts.destroyConfig(arktsGlobal.config);
      arkts.memFinalize();
    }
  }
}

function changeFileExtension(file: string, targetExt: string, originExt = ''): string {
  const currentExt = originExt.length === 0 ? path.extname(file) : originExt;
  const fileWithoutExt = file.substring(0, file.lastIndexOf(currentExt));
  return fileWithoutExt + targetExt;
}

function changeDeclgenFileExtension(file: string, targetExt: string): string {
  if (file.endsWith(common.fileUtils.Extension.DETS)) {
    return changeFileExtension(file, targetExt, common.fileUtils.Extension.DETS);
  }
  return changeFileExtension(file, targetExt);
}

function ensurePathExists(filePath: string): void {
  const dirPath = path.dirname(filePath);
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
  }
}

export function buildDeclgenOutputPath(
  inputFile: string,
  moduleInfo: common.fileManager.ModuleInfo,
  cacheDir?: string,
): { declEtsOutputPath: string } {
  const useCacheLayout = cacheDir !== undefined && inputFile.endsWith(ETSCACHE_SUFFIX);
  const filePathFromModuleRoot = useCacheLayout
    ? path.relative(cacheDir, inputFile)
    : path.relative(moduleInfo.modulePath, inputFile);

  const declEtsOutputPath = changeDeclgenFileExtension(
    useCacheLayout
      ? path.resolve(moduleInfo.declgenV1OutPath, filePathFromModuleRoot)
      : path.resolve(moduleInfo.declgenV1OutPath, filePathFromModuleRoot),
    common.fileUtils.Extension.DETS,
  );

  ensurePathExists(declEtsOutputPath);
  return { declEtsOutputPath };
}

function formDeclgenCliCmd(projectRootPath: string, arktsConfigPath: string): string[] {
  const ets2pandaCmd: string[] = ['_', '--extension', 'ets', '--arktsconfig', arktsConfigPath];

  ets2pandaCmd.push('--simultaneous');
  ets2pandaCmd.push('--ets-warnings:base-path=' + projectRootPath);

  return ets2pandaCmd;
}
