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

import * as path from 'path';
import * as fs from 'fs';

import { Logger, LogDataFactory, LogData } from '../logger';
import { ErrorCode } from '../util/error';
import { BuildConfig } from '../types';
import { ObfuscateExecutor } from './obfuscation_bytecode_executor';

export interface ObfuscatorData {
    directExecute: boolean;
    buildConfig: BuildConfig;
    arkGuardPath: string;
    configPath: string;
    debug: boolean;
    logger: Logger | undefined;
    obfuscatedOutputPath: string;
    mergedAbcFile: string;
}

export function printError(logger: Logger | undefined, logData: LogData): void {
    if (logger) {
        logger.printError(logData);
    } else {
        console.error(logData.toString());
    }
}

export function printDebug(logger: Logger | undefined, message: string): void {
    if (logger) {
        logger.printDebug(message);
    } else {
        console.debug(message);
    }
}

export function getObfuscatorDataPath(obfuscationCacheDir: string): string {
    return path.join(obfuscationCacheDir, 'obfuscatorDataFile.json');
}

export function existObfuscatorData(obfuscationCacheDir: string): boolean {
    return fs.existsSync(getObfuscatorDataPath(obfuscationCacheDir));
}

export function initObfuscatorDataStatus(obfuscationCacheDir: string): void {
    const obfuscatorDataPath: string = getObfuscatorDataPath(obfuscationCacheDir);
    if (fs.existsSync(obfuscatorDataPath)) {
        fs.unlinkSync(obfuscatorDataPath);
    }
}

export function writeObfuscatorData(obfuscationCacheDir: string, obfuscatorData: ObfuscatorData): void {
    const obfuscatorDataPath: string = getObfuscatorDataPath(obfuscationCacheDir);
    if (fs.existsSync(obfuscatorDataPath)) {
        fs.unlinkSync(obfuscatorDataPath);
    }
    fs.writeFileSync(obfuscatorDataPath, JSON.stringify(obfuscatorData, (key, value) => {
        if (typeof value === 'bigint') {
            return value.toString() + 'n';
        }
        return value;
    }, 2));
}

export function readObfuscatorData(obfuscationCacheDir: string): ObfuscatorData | undefined {
    const obfuscatorDataPath: string = getObfuscatorDataPath(obfuscationCacheDir);
    if (!fs.existsSync(obfuscatorDataPath)) {
        return undefined;
    }
    try {
        const obfuscatorDataContent: string = fs.readFileSync(obfuscatorDataPath).toString();
        return JSON.parse(obfuscatorDataContent);
    } catch {
        return undefined;
    }
}

export function executeObfuscator(obfuscatorData: ObfuscatorData): boolean {
    try {
        const result = ObfuscateExecutor.arkGuardExecute(obfuscatorData.arkGuardPath, obfuscatorData.configPath,
            obfuscatorData.debug, obfuscatorData.buildConfig, obfuscatorData.logger);
        if (!result.success) {
            const obfError: string = 'Obfuscate ABC file failed.';
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_OBFUSCATE_ABC_FAIL,
                obfError,
                obfError
            );
            printError(obfuscatorData.logger, logData);
            if (result.code !== undefined && result.description !== undefined && obfuscatorData.logger) {
                ObfuscateExecutor.printArkGuardError(result.code, result.description, obfuscatorData.logger);
            }
            return false;
        }
        printDebug(obfuscatorData.logger, 'bytecode obfuscation success.');
        const backupPath = ObfuscateExecutor.getOriginPath(obfuscatorData.obfuscatedOutputPath, 'origin');
        fs.copyFileSync(obfuscatorData.mergedAbcFile, backupPath);
        fs.copyFileSync(obfuscatorData.obfuscatedOutputPath, obfuscatorData.mergedAbcFile);
        if (obfuscatorData.debug) {
            const resultDisasm = ObfuscateExecutor.disAsmStaticExecute(obfuscatorData.buildConfig.disAsmStaticPath!,
                obfuscatorData.mergedAbcFile, obfuscatorData.obfuscatedOutputPath, obfuscatorData.logger);
            if (!resultDisasm.success) {
                const errorOutput = resultDisasm.error?.message || 'Unknown error';
                const logData = LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ARK_DISASM_FAIL,
                    'Disassemble obfuscated ABC file failed.',
                    errorOutput
                );
                printError(obfuscatorData.logger, logData);
                return false;
            }
            printDebug(obfuscatorData.logger, 'bytecode disassemble success.');
        }
        return true;
    } catch (error) {
        const logData = LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_OBFUSCATE_ABC_FAIL,
            'Obfuscate ABC file failed.',
            error instanceof Error ? error.message : String(error)
        );
        printError(obfuscatorData.logger, logData);
        return false;
    }
}

export function executeStaticObfuscatorFromHvigor(obfuscationCacheDir: string): void {
    if (!obfuscationCacheDir) {
        const logData = LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL,
            'Not get obfuscationCacheDir from hvigor'
        );
        printError(undefined, logData);
        return;
    }
    if (!existObfuscatorData(obfuscationCacheDir)) {
        return;
    }
    const obfuscatorData: ObfuscatorData | undefined = readObfuscatorData(obfuscationCacheDir);
    if (!obfuscatorData) {
        const logData = LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL,
            'Not get obfuscatorData'
        );
        printError(undefined, logData);
        return;
    }
    executeObfuscator(obfuscatorData);
    initObfuscatorDataStatus(obfuscationCacheDir);
}
