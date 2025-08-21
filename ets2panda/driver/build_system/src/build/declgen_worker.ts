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

import { CompileFileInfo, ModuleInfo } from '../types';
import { BuildConfig } from '../types';
import { Logger } from '../logger';
import * as fs from 'fs';
import * as path from 'path';
import {
  changeDeclgenFileExtension,
  changeFileExtension,
  createFileIfNotExists,
  ensurePathExists
} from '../utils';
import {
  DECL_ETS_SUFFIX,
  DECL_TS_SUFFIX,
  STATIC_RECORD_FILE,
  STATIC_RECORD_FILE_CONTENT,
  TS_SUFFIX
} from '../pre_define';
import { PluginDriver, PluginHook } from '../plugins/plugins_driver';
import { initKoalaModules } from '../init/init_koala_modules';

process.on('message', (message: {
  taskList: CompileFileInfo[];
  buildConfig: BuildConfig;
  moduleInfos: Array<[string, ModuleInfo]>;
}) => {
  if (!process.send) {
    throw new Error('process.send is undefined. This worker must be run as a forked process.');
  }
  const { taskList, buildConfig, moduleInfos } = message;
  const moduleInfosMap = new Map<string, ModuleInfo>(moduleInfos);

  const logger = Logger.getInstance(buildConfig);
  const pluginDriver = PluginDriver.getInstance();
  pluginDriver.initPlugins(buildConfig);

  let { arkts, arktsGlobal } = initKoalaModules(buildConfig)

  for (const fileInfo of taskList) {
    let errorStatus = false;
    try {
      const source = fs.readFileSync(fileInfo.filePath, 'utf8');
      let moduleInfo = moduleInfosMap.get(fileInfo.packageName)!;
      let filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, fileInfo.filePath);
      let declEtsOutputPath: string = path.join(
        moduleInfo.declgenV1OutPath as string,
        moduleInfo.packageName,
        filePathFromModuleRoot
      );
      declEtsOutputPath = changeDeclgenFileExtension(declEtsOutputPath, DECL_ETS_SUFFIX);
      let etsOutputPath: string = path.join(
        moduleInfo.declgenBridgeCodePath as string,
        moduleInfo.packageName,
        filePathFromModuleRoot
      );
      etsOutputPath = changeDeclgenFileExtension(etsOutputPath, TS_SUFFIX);

      ensurePathExists(declEtsOutputPath);
      ensurePathExists(etsOutputPath);

      const staticRecordPath = path.join(
        moduleInfo.declgenV1OutPath as string,
        STATIC_RECORD_FILE
      )
      const declEtsOutputDir = path.dirname(declEtsOutputPath);
      const staticRecordRelativePath = changeFileExtension(
        path.relative(declEtsOutputDir, staticRecordPath).replace(/\\/g, '\/'),
        '',
        DECL_TS_SUFFIX
      );
      createFileIfNotExists(staticRecordPath, STATIC_RECORD_FILE_CONTENT);

      arktsGlobal.filePath = fileInfo.filePath;
      arktsGlobal.config = arkts.Config.create([
        '_',
        '--extension',
        'ets',
        '--arktsconfig',
        fileInfo.arktsConfigFile,
        fileInfo.filePath
      ]).peer;
      arktsGlobal.compilerContext = arkts.Context.createFromStringWithHistory(source);
      pluginDriver.getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);
      const skipDeclCheck = buildConfig?.skipDeclCheck as boolean ?? true;

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer, skipDeclCheck);

      let ast = arkts.EtsScript.fromContext();
      pluginDriver.getPluginContext().setArkTSAst(ast);
      pluginDriver.runPluginHook(PluginHook.PARSED);

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer, skipDeclCheck);

      ast = arkts.EtsScript.fromContext();
      pluginDriver.getPluginContext().setArkTSAst(ast);
      pluginDriver.runPluginHook(PluginHook.CHECKED);

      arkts.generateTsDeclarationsFromContext(
        declEtsOutputPath,
        etsOutputPath,
        false,
        false,
        staticRecordRelativePath
      ); // Generate 1.0 declaration files & 1.0 glue code
      logger.printInfo('declaration files generated');

      process.send({ success: true, filePath: fileInfo.filePath });
    } catch (error) {
      errorStatus = true;
      if (error instanceof Error) {
        process.send({
          success: false,
          isDeclFile: true,
          filePath: fileInfo.filePath,
          error: 'Generate declaration files failed.\n' + error.message
        });
      }
    } finally {
      if (!errorStatus) {
        // when error occur,wrapper will destroy context.
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
      }
      arkts.destroyConfig(arktsGlobal.config);
    }
  }
  process.exit(0);
});