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
    changeFileExtension,
    ensurePathExists,
    formEts2pandaCmd
} from '../util/utils';
import {
    DECL_ETS_SUFFIX,
} from '../pre_define';
import { PluginDriver, PluginHook } from '../plugins/plugins_driver';
import {
    BuildConfig,
    CompileJobInfo,
    BUILD_MODE,
    OHOS_MODULE_TYPE
} from '../types';
import { initKoalaModules } from '../init/init_koala_modules';
import { LogDataFactory, Logger, getConsoleLogger } from '../logger';
import { ErrorCode, DriverError } from '../util/error';
import { KitImportTransformer } from '../plugins/KitImportTransformer'

process.on('message', (message: {
    job: CompileJobInfo;
    buildConfig: BuildConfig;
}) => {
    const { job, buildConfig } = message;

    Logger.getInstance(getConsoleLogger);
    PluginDriver.getInstance().initPlugins(buildConfig);
    let { arkts, arktsGlobal } = initKoalaModules(buildConfig)

    const isDebug = buildConfig.buildMode === BUILD_MODE.DEBUG;
    const ets2pandaCmd: string[] = formEts2pandaCmd(job, isDebug)

    const inputFile = job.compileFileInfo.inputFilePath
    const source = fs.readFileSync(inputFile).toString();

    try {
        arktsGlobal.filePath = inputFile;
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

        arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
        {
            const filePathFromModuleRoot = path.relative(buildConfig.moduleRootPath, inputFile);
            const declEtsOutputPath = changeFileExtension(
                path.join(buildConfig.declgenV2OutPath!, filePathFromModuleRoot),
                DECL_ETS_SUFFIX
            );
            ensurePathExists(declEtsOutputPath);
            arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
        }
        PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

        arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);

        process.send!(job);
    } catch (error) {
        if (error instanceof Error) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                    'Compile abc files failed.',
                    error.message,
                    inputFile
                )
            );
        }
    } finally {
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
        PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
        arkts.destroyConfig(arktsGlobal.config);
    }
});
