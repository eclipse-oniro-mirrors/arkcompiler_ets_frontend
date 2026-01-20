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
    Logger.getInstance(hvigorLogger ?? (loggerGetter ?? getConsoleLogger));
}

function replaceOnEnv(str: string, indexA: number, indexB: number): string {
    let envName: string = str.substring(indexA + 2, indexB)
    const envValue = process.env[envName] || ''

    if (envValue === '') {
        throw new Error(envName + ' environment variable is not set');
    }

    return str.replace(str.substring(indexA, indexB + 1), envValue)
}

function getVar(str: string | null | undefined): string {
    if (str === null || str === undefined || str === '') {
        return ''
    }
    let indexA = str.indexOf('$')
    let indexB = str.indexOf('}')

    if (indexA === -1 || indexB === -1) {
        return str
    }

    return replaceOnEnv(str, indexA, indexB)
}

function validatePlugins(projectConfig: BuildConfig): void {
    for (const key in projectConfig.plugins) {
        projectConfig.plugins[key] = getVar(projectConfig.plugins[key])
    }
}

function validatePaths(projectConfig: BuildConfig): void {
    for (const key in projectConfig.paths) {
        for (let i = 0; i < projectConfig.paths[key].length; i++) {
            projectConfig.paths[key][i] = getVar(projectConfig.paths[key][i])
        }
    }
}

function validateSingleFieldPaths(projectConfig: BuildConfig): void {
    if (projectConfig.pandaStdlibPath) {
        projectConfig.pandaStdlibPath = getVar(projectConfig.pandaStdlibPath)
    }
    
    projectConfig.sdkAliasMap = projectConfig.sdkAliasMap instanceof Map
        ? projectConfig.sdkAliasMap
        : new Map(Object.entries(projectConfig.sdkAliasMap || {}));

    projectConfig.moduleRootPath = getVar(projectConfig.moduleRootPath)
    projectConfig.buildSdkPath = getVar(projectConfig.buildSdkPath)

    projectConfig.entryFile = getVar(projectConfig.entryFile);
}

function validateInteropApiPaths(interopApiPaths: string[]): void {
    for (let i = 0; i < interopApiPaths?.length || 0; i++) {
        interopApiPaths[i] = getVar(interopApiPaths[i])
    }
}

function validateSdkAliasMap(projectConfig: BuildConfig): void {
    if (projectConfig.sdkAliasMap.size !== 0) {
        for (const [name, path] of projectConfig.sdkAliasMap) {
            const newPath = getVar(path);
            projectConfig.sdkAliasMap.set(name, newPath);
        }
    }
}

function validateCompileFiles(compileFiles: string[]): void {
    compileFiles.forEach((file, i) => {
        compileFiles[i] = getVar(file);
    });
}

function validateDependencyModuleList(projectConfig: BuildConfig): void {
    for (let i = 0; i < projectConfig.dependencyModuleList?.length || 0; i++) {
        projectConfig.dependencyModuleList[i].modulePath = getVar(projectConfig.dependencyModuleList[i].modulePath)

        const currentDeclFilesPath = projectConfig.dependencyModuleList[i].declFilesPath;
        if (currentDeclFilesPath) {
            projectConfig.dependencyModuleList[i].declFilesPath = getVar(currentDeclFilesPath)
        }

        for (let j = 0; j < projectConfig.dependencyModuleList[i].sourceRoots.length; j++) {
            projectConfig.dependencyModuleList[i].sourceRoots[j] = getVar(projectConfig.dependencyModuleList[i].sourceRoots[j])
        }
    }
}

function validateDependentModuleList(projectConfig: BuildConfig): void {
    for (let i = 0; i < projectConfig.dependentModuleList?.length || 0; i++) {
        projectConfig.dependentModuleList[i].modulePath = getVar(projectConfig.dependentModuleList[i].modulePath)

        const currentDeclFilesPath = projectConfig.dependentModuleList[i].declFilesPath;
        if (currentDeclFilesPath) {
            projectConfig.dependentModuleList[i].declFilesPath = getVar(currentDeclFilesPath)
        }

        for (let j = 0; j < projectConfig.dependentModuleList[i].sourceRoots.length; j++) {
            projectConfig.dependentModuleList[i].sourceRoots[j] = getVar(projectConfig.dependentModuleList[i].sourceRoots[j])
        }
    }
}

function validateModuleFiles(projectConfig: BuildConfig): void {
    for (let i = 0; i < projectConfig.moduleFiles?.length || 0; i++) {
        projectConfig.moduleFiles[i].packageName = getVar(projectConfig.moduleFiles[i].packageName);
        for (let j = 0; j < projectConfig.moduleFiles[i].staticFiles?.length || 0; j++) {
            projectConfig.moduleFiles[i].staticFiles[j] = getVar(projectConfig.moduleFiles[i].staticFiles[j]);
        }
    }
}

function validate(projectConfig: BuildConfig): void {
    validatePlugins(projectConfig)
    validatePaths(projectConfig)
    validateSingleFieldPaths(projectConfig)
    validateInteropApiPaths(projectConfig.interopApiPaths)
    validateSdkAliasMap(projectConfig)
    validateCompileFiles(projectConfig.compileFiles)
    validateDependencyModuleList(projectConfig)
    validateDependentModuleList(projectConfig)
    validateModuleFiles(projectConfig)
}

export async function build(projectConfig: BuildConfig, loggerGetter?: LoggerGetter): Promise<void> {
    validate(projectConfig)
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
