/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
import * as os from 'os';
import * as path from 'path';

import { LogData, LogDataFactory } from '../logger';
import { BuildConfig } from '../types';
import { ErrorCode } from '../util/error';
import { isMac, isWindows } from '../util/utils';
import { ObfuscateExecutor } from './obfuscation_bytecode_executor';

export const EMPTY_AOP_ABC_PATH_SENTINEL = 'ONLY_USE_BY_EMPTY_AOP_PATH';

export type TransformLibInstrumentParam = {
    buildSdkPath: string;
    libPath: string;
    dynamic: {
        srcPath: string;
        dstPath: string;
    };
    static: {
        srcPath: string;
        dstPath: string;
    };
};

type TransformInputPaths = {
    dynamicPathForConfig: string;
    staticPathForConfig: string;
};

let transformLibEnvInited: boolean = false;
let arkGuardPath: string = '';

function printError(error: LogData): void {
    console.error(error.toString());
}

function fail(code: ErrorCode, description: string, cause = '', position = ''): void {
    printError(LogDataFactory.newInstance(code, description, cause, position));
}

export function getArkGuardPathFromBuildSdk(buildSdkPath: string): string {
    const resolved: string = path.resolve(buildSdkPath);
    return path.join(resolved, 'build-tools', 'ets2panda', 'bin', isWindows() ? 'ark_guard.exe' : 'ark_guard');
}

export function initTransformLibEnv(projectConfig: BuildConfig): void {
    if (!projectConfig || !projectConfig.buildSdkPath) {
        fail(ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL, `projectConfig's attribute 'buildSdkPath' is not exist.`);
        return;
    }
    arkGuardPath = getArkGuardPathFromBuildSdk(projectConfig.buildSdkPath as string);
    transformLibEnvInited = true;
}

function resolveGuardPath(param: TransformLibInstrumentParam): string | undefined {
    if (transformLibEnvInited) {
        return arkGuardPath;
    }
    if (!param || !param.buildSdkPath) {
        fail(
            ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL,
            `The transformLib's env is not initialized. Also, param's attribute 'buildSdkPath' does not exist.`
        );
        return undefined;
    }
    return getArkGuardPathFromBuildSdk(param.buildSdkPath);
}

function validateInputs(param: TransformLibInstrumentParam, guardPath: string): boolean {
    if (!param.libPath) {
        fail(ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL, `libPath's value is empty.`);
        return false;
    }
    if (!fs.existsSync(guardPath)) {
        fail(
            ErrorCode.BUILDSYSTEM_ARK_GUARD_NOT_FOUND_FAIL,
            `ArkGuard executable not found for transform lib step.`,
            '',
            guardPath
        );
        return false;
    }
    return true;
}

function copySrcToDst(srcPath: string, dstPath: string): boolean {
    if (srcPath === dstPath) {
        return true;
    }
    try {
        fs.copyFileSync(srcPath, dstPath);
    } catch (error) {
        fail(
            ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL,
            `Failed to copy file from ${srcPath} to ${dstPath}.`,
            error instanceof Error ? error.message : String(error)
        );
        return false;
    }
    return true;
}

function collectAopAbcPaths(param: TransformLibInstrumentParam): TransformInputPaths | undefined {
    let dynamicPathForConfig: string = EMPTY_AOP_ABC_PATH_SENTINEL;
    let staticPathForConfig: string = EMPTY_AOP_ABC_PATH_SENTINEL;
    let existAbcFilePath: boolean = false;

    if (param.dynamic.srcPath && param.dynamic.dstPath && fs.existsSync(param.dynamic.srcPath)) {
        if (!copySrcToDst(param.dynamic.srcPath, param.dynamic.dstPath)) {
            return undefined;
        }
        dynamicPathForConfig = param.dynamic.dstPath;
        existAbcFilePath = true;
    }

    if (param.static.srcPath && param.static.dstPath && fs.existsSync(param.static.srcPath)) {
        if (!copySrcToDst(param.static.srcPath, param.static.dstPath)) {
            return undefined;
        }
        staticPathForConfig = param.static.dstPath;
        existAbcFilePath = true;
    }

    if (!existAbcFilePath) {
        fail(ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL, `Both dynamic file path and static file path are empty.`);
        return undefined;
    }

    return { dynamicPathForConfig, staticPathForConfig };
}

function buildAopConfig(
    param: TransformLibInstrumentParam,
    paths: TransformInputPaths
): { configDir: string; configPath: string } {
    const stubAbcPath: string =
        paths.dynamicPathForConfig !== EMPTY_AOP_ABC_PATH_SENTINEL
            ? paths.dynamicPathForConfig
            : paths.staticPathForConfig;
    const stubNameCachePath: string = path.join(os.tmpdir(), 'ark_guard_transform_stub_namecache.json');
    const configObj: Record<string, unknown> = {
        abcPath: stubAbcPath,
        obfAbcPath: stubAbcPath,
        defaultNameCachePath: stubNameCachePath,
        transformLib: path.resolve(param.libPath),
        dynamicAbcPath: paths.dynamicPathForConfig,
        staticAbcPath: paths.staticPathForConfig,
        obfuscationRules: {
            disableObfuscation: true,
        },
    };

    const configDir: string = fs.mkdtempSync(path.join(os.tmpdir(), 'ark_guard_transform_'));
    const configPath: string = path.join(configDir, 'aop_config.json');
    fs.writeFileSync(configPath, JSON.stringify(configObj, null, 2));
    return { configDir, configPath };
}

function executeAopTransform(guardPath: string, configPath: string): void {
    const env: NodeJS.ProcessEnv | undefined = isMac()
        ? { ...process.env, DYLD_LIBRARY_PATH: process.env.DYLD_LIBRARY_PATH || '' }
        : undefined;
    const result = ObfuscateExecutor.executeCommandWithOutput(guardPath, [configPath], env);
    if (!result.success) {
        fail(
            ErrorCode.BUILDSYSTEM_TRANSFORM_LIB_FAIL,
            `Execute ark_guard (transform lib) failed.`,
            result.description || result.error?.message || ''
        );
    }
}

function cleanupTempConfig(configDir: string, configPath: string): void {
    try {
        fs.unlinkSync(configPath);
        fs.rmdirSync(configDir);
    } catch {
        // ignore cleanup errors
    }
}

export function transformLib(param: TransformLibInstrumentParam): void {
    const guardPath = resolveGuardPath(param);
    if (!guardPath || !validateInputs(param, guardPath)) {
        return;
    }

    const inputPaths = collectAopAbcPaths(param);
    if (!inputPaths) {
        return;
    }

    const { configDir, configPath } = buildAopConfig(param, inputPaths);
    try {
        executeAopTransform(guardPath, configPath);
    } finally {
        cleanupTempConfig(configDir, configPath);
    }
}
