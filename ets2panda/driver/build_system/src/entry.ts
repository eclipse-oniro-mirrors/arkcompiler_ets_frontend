/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { initBuildConfig } from './init/process_build_config';
import { BuildMode } from './build/build_mode';
import { Logger, LoggerGetter, getConsoleLogger } from './logger';
import { DriverError } from './util/error';
import { isMac, substituteEnvVarsInJSON } from './util/utils';
import { PluginDriver } from './plugins/plugins_driver';
import { BuildConfig, BUILD_TYPE } from './types';
import { BuildFrameworkMode } from './build/build_framework_mode';
import { cleanKoalaModule } from './init/init_koala_modules';
import { buildForMac } from './entry_mac';

// NOTE: to be refactored
function backwardCompatibleBuildConfigStub(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): void {
    if (projectConfig.dependentModuleList) {
        projectConfig.dependencyModuleList = [...projectConfig.dependentModuleList]
    }

    const hvigorLogger = projectConfig.getHvigorConsoleLogger as LoggerGetter
    Logger.getInstance(hvigorLogger ?? (loggerGetter ?? getConsoleLogger), projectConfig.enableDebugOutput);
}

export async function runBuild(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): Promise<void> {
    backwardCompatibleBuildConfigStub(projectConfig, loggerGetter)

    let logger: Logger = Logger.getInstance();
    logger.printDebug(`Project config: ${JSON.stringify(projectConfig, null, 1)}`)

    let buildConfig: BuildConfig = initBuildConfig(projectConfig);

    try {
        if (projectConfig.frameworkMode === true) {
            let buildframeworkMode: BuildFrameworkMode = new BuildFrameworkMode(buildConfig);
            await buildframeworkMode.runSimultaneous();
        } else {
            let buildMode: BuildMode = new BuildMode(buildConfig);
            if (projectConfig.enableDeclgenEts2Ts === true) {
                logger.printInfo('generate Declaration')
                await buildMode.generateDeclarationV1Parallel();
            } else if (projectConfig.buildType === BUILD_TYPE.BUILD ||
                buildConfig.isLocalTest || buildConfig.isOhosTest) {
                logger.printInfo('just build')
                await buildMode.run();
            }
        }
    } catch (error) {
        if (error instanceof DriverError) {
            Logger.getInstance().printErrorAndExit((error as DriverError).logData);
            throw error;
        } else {
            Logger.getInstance().printWarn('Error occured');
            Logger.getInstance().printWarn('Error is not DriverError');
            throw error;
        }
    } finally {
        clean();
    }
}

function clean(): void {
    Logger.destroyInstance();
    PluginDriver.destroyInstance();
    cleanKoalaModule();
}

export async function build(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): Promise<void> {
    // execute build in child process for mac platform
    // In mac platform with daemon mode , dlopen will success when first compiled in one process
    // In the following compiled step , dlopen will fail
    // So we make unique process with mac build
    if (isMac()) {
        return buildForMac(projectConfig, loggerGetter);
    }
    return runBuild(projectConfig, loggerGetter);
}

function main(): void {
    const buildConfigPath: string = path.resolve(process.argv[2]);
    const projectConfig: BuildConfig = substituteEnvVarsInJSON(JSON.parse(fs.readFileSync(buildConfigPath, 'utf-8')));
    build(projectConfig);
}

if (require.main === module) {
    main();
}
