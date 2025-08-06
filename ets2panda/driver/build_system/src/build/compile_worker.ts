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
import * as fs from 'fs';
import * as path from 'path';

import {
  changeFileExtension,
  ensurePathExists,
  serializeWithIgnore
} from '../util/utils';
import {
  DECL_ETS_SUFFIX,
  KOALA_WRAPPER_PATH_FROM_SDK
} from '../pre_define';
import { PluginDriver, PluginHook } from '../plugins/plugins_driver';
import {
  BuildConfig,
  BUILD_MODE,
  OHOS_MODULE_TYPE
} from '../types';
import { initKoalaModules } from '../init/init_koala_modules';
import { LogData, LogDataFactory, Logger } from '../logger';
import { ErrorCode } from '../error_code';
import { KitImportTransformer } from '../plugins/KitImportTransformer'

process.on('message', async (message: {
  id: number;
  fileInfo: CompileFileInfo;
  buildConfig: BuildConfig;
}) => {
  const id = message.id;
  //@ts-ignore
  const { fileInfo, buildConfig } = message.payload;

  Logger.getInstance(buildConfig);
  PluginDriver.getInstance().initPlugins(buildConfig);
  let { arkts, arktsGlobal } = initKoalaModules(buildConfig)
  let errorStatus = false;

  try {
    Logger.getInstance(buildConfig);
    PluginDriver.getInstance().initPlugins(buildConfig);
    const isDebug = buildConfig.buildMode === BUILD_MODE.DEBUG;

    ensurePathExists(fileInfo.abcFilePath);
    const source = fs.readFileSync(fileInfo.filePath).toString();

    let ets2pandaCmd = [
      '_', '--extension', 'ets',
      '--arktsconfig', fileInfo.arktsConfigFile,
      '--output', fileInfo.abcFilePath
    ];
    if (isDebug) {
      ets2pandaCmd.push('--debug-info');
      ets2pandaCmd.push('--opt-level=0');
    }
    ets2pandaCmd.push(fileInfo.filePath);

    arktsGlobal.filePath = fileInfo.filePath;
    arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
    arktsGlobal.compilerContext = arkts.Context.createFromString(source);

    PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
    if (buildConfig.aliasConfig && Object.keys(buildConfig.aliasConfig).length > 0) {
      // if aliasConfig is set, transform aliasName@kit.xxx to default@ohos.xxx through the plugin
      let ast = arkts.EtsScript.fromContext();
      let transformAst = new KitImportTransformer(
        arkts,
        arktsGlobal.compilerContext.program,
        buildConfig.buildSdkPath,
        buildConfig.aliasConfig
      ).transform(ast);
      PluginDriver.getInstance().getPluginContext().setArkTSAst(transformAst);
    }
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

    if (buildConfig.hasMainModule && (buildConfig.byteCodeHar || buildConfig.moduleType === OHOS_MODULE_TYPE.SHARED)) {
      const filePathFromModuleRoot = path.relative(buildConfig.moduleRootPath, fileInfo.filePath);
      const declEtsOutputPath = changeFileExtension(
        path.join(buildConfig.declgenV2OutPath!, filePathFromModuleRoot),
        DECL_ETS_SUFFIX
      );
      ensurePathExists(declEtsOutputPath);
      arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
    }
    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
    PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);

    if (process.send) {
      process.send({ id, success: true });
    }
  } catch (error) {
    errorStatus = true;
    if (error instanceof Error) {
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Compile abc files failed.',
        error.message,
        fileInfo.filePath
      );
      if (process.send) {
        process.send({
          id,
          success: false,
          error: serializeWithIgnore(logData)
        });
      }
    }
  } finally {
    if (!errorStatus) {
      arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
    }
    PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    arkts.destroyConfig(arktsGlobal.config);
  }
});
