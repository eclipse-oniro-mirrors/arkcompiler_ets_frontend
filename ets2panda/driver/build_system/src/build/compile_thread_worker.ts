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
import * as path from 'path';
import {
    changeFileExtension,
    ensurePathExists
} from '../util/utils';
import {
    DECL_ETS_SUFFIX,
} from '../pre_define';
import { PluginDriver, PluginHook } from '../plugins/plugins_driver';
import {
    BuildConfig,
    BUILD_MODE,
    OHOS_MODULE_TYPE,
    KPointer,
    CompileJobInfo
} from '../types';
import {
    LogData,
    LogDataFactory,
    Logger,
} from '../logger';
import { ErrorCode } from '../util/error';
import { KitImportTransformer } from '../plugins/KitImportTransformer';
import { initKoalaModules } from '../init/init_koala_modules';

const { workerId } = workerData;

function compileAbc(jobInfo: CompileJobInfo, globalContextPtr: KPointer, buildConfig: BuildConfig): void {
    PluginDriver.getInstance().initPlugins(buildConfig);
    let { arkts, arktsGlobal } = initKoalaModules(buildConfig)
    const isDebug = buildConfig.buildMode === BUILD_MODE.DEBUG;

    let errorStatus = false;
    try {
        let fileInfo = jobInfo.compileFileInfo;
        ensurePathExists(fileInfo.inputFilePath);

        const ets2pandaCmd = [
            '_', '--extension', 'ets',
            '--arktsconfig', fileInfo.arktsConfigFile,
            '--output', fileInfo.inputFilePath,
        ];

        if (isDebug) {
            ets2pandaCmd.push('--debug-info');
            ets2pandaCmd.push('--opt-level=0');
        }
        ets2pandaCmd.push(fileInfo.inputFilePath);

        let arkConfig = arkts.Config.create(ets2pandaCmd).peer;
        arktsGlobal.config = arkConfig;

        let context = arkts.Context.createCacheContextFromFile(arkConfig, fileInfo.inputFilePath, globalContextPtr, false).peer;

        PluginDriver.getInstance().getPluginContext().setContextPtr(context);

        arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, context);
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

        arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, context);

        {
            let filePathFromModuleRoot: string = path.relative(buildConfig.moduleRootPath, fileInfo.inputFilePath);
            let declEtsOutputPath: string = changeFileExtension(
                path.join(buildConfig.declgenV2OutPath as string, filePathFromModuleRoot),
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

function compileDeclaration(jobInfo: CompileJobInfo, globalContextPtr: KPointer, buildConfig: BuildConfig): void {
    PluginDriver.getInstance().initPlugins(buildConfig);
    let { arkts, arktsGlobal } = initKoalaModules(buildConfig)
    const isDebug = buildConfig.buildMode === BUILD_MODE.DEBUG;

    let errorStatus = false;
    try {
        let fileInfo = jobInfo.compileFileInfo;
        const ets2pandaCmd = ['-', '--extension', 'ets', '--arktsconfig', fileInfo.arktsConfigFile];

        if (isDebug) {
            ets2pandaCmd.push('--debug-info');
            ets2pandaCmd.push('--opt-level=0');
        }
        ets2pandaCmd.push(fileInfo.inputFilePath);

        let arkConfig = arkts.Config.create(ets2pandaCmd).peer;
        arktsGlobal.config = arkConfig;

        let context = arkts.Context.createCacheContextFromFile(arkConfig, fileInfo.inputFilePath, globalContextPtr, true).peer;

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

parentPort!.on('message', (msg: any) => {
    if (msg.type === 'ASSIGN_TASK') {
        const { job, globalContextPtr, buildConfig } = msg.data;

        if (job.isCompileAbc) {
            compileAbc(job, globalContextPtr, buildConfig);
        } else {
            compileDeclaration(job, globalContextPtr, buildConfig);
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
