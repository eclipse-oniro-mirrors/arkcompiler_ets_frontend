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

import { parentPort, workerData } from 'worker_threads';
import { JobInfo } from '../types';
import * as path from 'path';
import {
  changeFileExtension,
  ensurePathExists
} from '../utils';
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
import {
  LogData,
  LogDataFactory,
  Logger
} from '../logger';
import { ErrorCode } from '../error_code';
import { KitImportTransformer } from '../plugins/KitImportTransformer';
import { initKoalaModules } from '../init/init_koala_modules';

const { workerId } = workerData;

function compileAbc(jobInfo: JobInfo): void {
  let config = jobInfo.buildConfig as BuildConfig;
  Logger.getInstance(config);
  PluginDriver.getInstance().initPlugins(config);
  let { arkts, arktsGlobal } = initKoalaModules(config)
  const isDebug = config.buildMode === BUILD_MODE.DEBUG;

  let errorStatus = false;
  try {
    let fileInfo = jobInfo.compileFileInfo;
    ensurePathExists(fileInfo.abcFilePath);

    const ets2pandaCmd = [
      '_', '--extension', 'ets',
      '--arktsconfig', fileInfo.arktsConfigFile,
      '--output', fileInfo.abcFilePath,
    ];

    if (isDebug) {
      ets2pandaCmd.push('--debug-info');
      ets2pandaCmd.push('--opt-level=0');
    }
    ets2pandaCmd.push(fileInfo.filePath);

    let arkConfig = arkts.Config.create(ets2pandaCmd).peer;
    arktsGlobal.config = arkConfig;

    let context = arkts.Context.createCacheContextFromFile(arkConfig, fileInfo.filePath, jobInfo.globalContextPtr, false).peer;

    PluginDriver.getInstance().getPluginContext().setContextPtr(context);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, context);
    if (config.aliasConfig && Object.keys(config.aliasConfig).length > 0) {
      // if aliasConfig is set, transform aliasName@kit.xxx to default@ohos.xxx through the plugin
      let ast = arkts.EtsScript.fromContext();
      let transformAst = new KitImportTransformer(
        arkts,
        arktsGlobal.compilerContext.program,
        config.buildSdkPath,
        config.aliasConfig
      ).transform(ast);
      PluginDriver.getInstance().getPluginContext().setArkTSAst(transformAst);
    }
    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, context);

    if (config.hasMainModule && (config.byteCodeHar || config.moduleType === OHOS_MODULE_TYPE.SHARED)) {
      let filePathFromModuleRoot: string = path.relative(config.moduleRootPath, fileInfo.filePath);
      let declEtsOutputPath: string = changeFileExtension(
        path.join(config.declgenV2OutPath as string, filePathFromModuleRoot),
        DECL_ETS_SUFFIX
      );
      ensurePathExists(declEtsOutputPath);

      // Generate 1.2 declaration files(a temporary solution while binary import not pushed)
      arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
    }

    PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, context);
  } catch (error) {
    errorStatus = true;
    if (error instanceof Error) {
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Compile abc files failed.',
        error.message
      );
      Logger.getInstance().printError(logData);
    }
  } finally {
    if (!errorStatus) {
      // when error occur,wrapper will destroy context.
      arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
    }
    PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    arkts.destroyConfig(arktsGlobal.config);
  }
}

function compileExternalProgram(jobInfo: JobInfo): void {
  let config = jobInfo.buildConfig as BuildConfig;
  Logger.getInstance(config);
  PluginDriver.getInstance().initPlugins(config);
  let { arkts, arktsGlobal } = initKoalaModules(config)
  const isDebug = config.buildMode === BUILD_MODE.DEBUG;

  let errorStatus = false;
  try {
    let fileInfo = jobInfo.compileFileInfo;
    const ets2pandaCmd = ['-', '--extension', 'ets', '--arktsconfig', fileInfo.arktsConfigFile];

    if (isDebug) {
      ets2pandaCmd.push('--debug-info');
      ets2pandaCmd.push('--opt-level=0');
    }
    ets2pandaCmd.push(fileInfo.filePath);

    let arkConfig = arkts.Config.create(ets2pandaCmd).peer;
    arktsGlobal.config = arkConfig;

    let context = arkts.Context.createCacheContextFromFile(arkConfig, fileInfo.filePath, jobInfo.globalContextPtr, true).peer;

    PluginDriver.getInstance().getPluginContext().setContextPtr(context);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, context);

    PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, context);

    PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

    arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_LOWERED, context);
  } catch (error) {
    errorStatus = true;
    if (error instanceof Error) {
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Compile external program files failed.',
        error.message
      );
      Logger.getInstance().printError(logData);
    }
  } finally {
    if (!errorStatus) {
      // when error occur,wrapper will destroy context.
      arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
    }
    PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
    arkts.destroyConfig(arktsGlobal.config);
  }
}

parentPort?.on('message', (msg) => {
  if (msg.type === 'ASSIGN_TASK') {
      const job = msg.jobInfo;

      if (job.isCompileAbc) {
        compileAbc(job);
      } else {
        compileExternalProgram(job);
      }
      
      parentPort?.postMessage({
          type: 'TASK_FINISH',
          jobId: job.id,
          workerId,
      });
  } else if (msg.type === 'EXIT') {
      process.exit(0);
  }
});
