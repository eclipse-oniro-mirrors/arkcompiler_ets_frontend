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

import { initBuildConfig } from './init/process_build_config';
import { BuildMode } from './build/build_mode';
import { Logger, LoggerGetter, getConsoleLogger } from './logger';
import { DriverError } from './util/error';
import { ArkTSConfigGenerator } from './build/generate_arktsconfig';
import { PluginDriver } from './plugins/plugins_driver';
import { BuildConfig, BUILD_TYPE } from './types';
import { BuildFrameworkMode } from './build/build_framework_mode';
import { cleanKoalaModule } from './init/init_koala_modules';

// NOTE: to be refactored
function backwardCompatibleBuildConfigStub(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): void {
    if (projectConfig.dependentModuleList) {
        projectConfig.dependencyModuleList = [...projectConfig.dependentModuleList]
    }

    const hvigorLogger = projectConfig.getHvigorConsoleLogger as LoggerGetter
    delete projectConfig.getHvigorConsoleLogger
    Logger.getInstance(hvigorLogger ?? (loggerGetter ?? getConsoleLogger));
}

export async function build(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): Promise<void> {
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
            } else if (projectConfig.buildType === BUILD_TYPE.BUILD) {
                logger.printInfo('just build')
                await buildMode.run();
            }
        }
    } catch (error) {
        if (error instanceof DriverError) {
            Logger.getInstance().printErrorAndExit((error as DriverError).logData);
        } else {
            Logger.getInstance().printWarn('Error occured')
            Logger.getInstance().printWarn('Error is not DriverError')
            throw error;
        }
    } finally {
        clean();
    }
}

function clean(): void {
    Logger.destroyInstance();
    ArkTSConfigGenerator.destroyInstance();
    PluginDriver.destroyInstance();
    cleanKoalaModule();
}

function main(): void {
    const buildConfigPath: string = path.resolve(process.argv[2]);
    const projectConfig: BuildConfig = JSON.parse(fs.readFileSync(buildConfigPath, 'utf-8'));

    build(projectConfig)
}

if (require.main === module) {
    main();
}
