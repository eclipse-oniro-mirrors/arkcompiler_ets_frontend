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

import * as path from 'path';
import * as fs from 'fs';
import * as child_process from 'child_process';

import { Logger, LogDataFactory } from '../logger';
import { ErrorCode } from '../util/error';
import { BuildConfig } from '../types';
import { isMac } from '../util/utils';
import {
    executeObfuscator,
    initObfuscatorDataStatus,
    ObfuscatorData,
    printDebug,
    writeObfuscatorData
} from './obfuscation_bytecode_executor_invoke';

export interface ObfuscateExecuteParams {
    buildConfig: BuildConfig;
    logger: Logger;
    mergedAbcFile: string;
    arkGuardPath: string | undefined;
}

export interface SpawnSyncResult {
    success: boolean;
    code?: ErrorCode;
    description?: string;
    error?: Error;
}

export class ObfuscateExecutor {

    public static getOriginPath(obfPath: string, dirName: string): string {
        const fileName = path.basename(obfPath);
        const grandParentDir = path.dirname(path.dirname(obfPath));
        const originDir = path.join(grandParentDir, dirName);
        if (!fs.existsSync(originDir)) {
            fs.mkdirSync(originDir, { recursive: true });
        }
        return path.join(originDir, fileName);
    }

    private static getExecutionEnvironment(): NodeJS.ProcessEnv | undefined {
        if (isMac()) {
            return { ...process.env, DYLD_LIBRARY_PATH: process.env.DYLD_LIBRARY_PATH || '' };
        }
        return undefined;
    }


    public static executeCommandWithOutput(command: string, args: string[], env?: NodeJS.ProcessEnv): SpawnSyncResult {
        try {
            const options: child_process.SpawnSyncOptionsWithStringEncoding = {
                encoding: 'utf8'
            };

            if (env) {
                options.env = env;
            } else if (isMac()) {
                options.env = ObfuscateExecutor.getExecutionEnvironment();
            }

            const result = child_process.spawnSync(command, args, options);
            const rstErr = result.stderr?.toString().trim();
            const hasError = !(result.error || rstErr);
            let spawnResult: SpawnSyncResult = {
                success: hasError
            }
            if (!hasError) {
                const match = rstErr.match(/\[ErrorCode\]:(\d+)[\s\S]*?\[Description\]:([\s\S]*?)(?:\n|$)/i);
                if (match && match[1] && match[2]) {
                    const errstr: string = match[1].trim();
                    spawnResult.code = Object.values(ErrorCode).find(value => value === errstr) as ErrorCode;
                    spawnResult.description = match[2].trim();
                }
            }
            return spawnResult;
        } catch (error) {
            return {
                success: false,
                error: error instanceof Error ? error : new Error(String(error))
            };
        }
    }

    public static arkGuardExecute(arkGuardPath: string, configPath: string, debug: boolean | undefined,
        buildConfig: BuildConfig, logger: Logger | undefined): SpawnSyncResult {
        const args = [arkGuardPath];
        if (debug) {
            args.push('--debug');
            if (buildConfig.obfuscationOptions && buildConfig.obfuscationOptions.obfuscationCacheDir) {
                const debugFilePath = path.join(buildConfig.obfuscationOptions.obfuscationCacheDir, 'debug.txt');
                args.push('--debug-file');
                args.push(debugFilePath);
            }
        }
        args.push(configPath);

        const env = ObfuscateExecutor.getExecutionEnvironment();

        printDebug(logger, `ArkGuard command: ${args.map(arg => `"${arg}"`).join(' ')}`);

        return ObfuscateExecutor.executeCommandWithOutput(args[0]!, args.slice(1), env);
    }

    public static disAsmStaticExecute(disAsmPath: string, abcFile: string, disAsmOutPath: string,
        logger: Logger | undefined): SpawnSyncResult {
        const args = [disAsmPath];
        args.push(abcFile);
        const disAsmOutFile = path.join(path.dirname(disAsmOutPath), path.basename(abcFile, path.extname(abcFile)) + '.pa');
        args.push(disAsmOutFile);

        let env: NodeJS.ProcessEnv | undefined;
        if (isMac()) {
            env = { ...process.env, DYLD_LIBRARY_PATH: process.env.DYLD_LIBRARY_PATH || '' };
        }

        printDebug(logger, `disAsm command: ${args.map(arg => `"${arg}"`).join(' ')}`);

        return ObfuscateExecutor.executeCommandWithOutput(args[0]!, args.slice(1), env);
    }

    public static printArkGuardError(errorCode: ErrorCode, description: string, logger: Logger): void {
        const logData = LogDataFactory.newInstance(
            errorCode,
            'Obfuscate ark_guard tool failed.',
            description
        );
        logger.printError(logData);
    }

    public static obfuscateExecute(params: ObfuscateExecuteParams): boolean {
        const { buildConfig, logger, mergedAbcFile, arkGuardPath } = params;
        const mergedObConfig = buildConfig.mergedObConfig;
        if (!mergedObConfig) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL,
                `Obfuscation configuration is missing.` +
                '\nSolutions: Ensure obfuscation configuration is properly generated before execution.'
            );
            logger.printError(logData);
            return false;
        }

        if (!arkGuardPath || !fs.existsSync(arkGuardPath)) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ARK_GUARD_NOT_FOUND_FAIL,
                `ArkGuard executable not found.` +
                `\nCurrent ArkGuard Path: ${arkGuardPath}` +
                `\nSolutions: Ensure the ArkGuard tool exists under the SDK path configured for the project.`
            );
            logger.printError(logData);
            return false;
        }

        if (!buildConfig.bytecodeObfuscationConfig || !buildConfig.bytecodeObfuscationConfig.obfAbcPath) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL,
                `Bytecode obfuscation configuration is undefined. ` +
                '\nSolutions: Ensure obfuscation configuration is properly generated before execution.'
            );
            logger.printError(logData);
            return false;
        }
        const obfuscatedOutputPath: string = buildConfig.bytecodeObfuscationConfig.obfAbcPath;
        const configPath: string = mergedObConfig.options.bytecodeObf.configPath;
        const debug: boolean | undefined = mergedObConfig.options.bytecodeObf.debugging;

        if (!buildConfig.obfuscationOptions || !buildConfig.obfuscationOptions.obfuscationCacheDir) {
            return false;
        }
        initObfuscatorDataStatus(buildConfig.obfuscationOptions.obfuscationCacheDir);

        const obfuscatorData: ObfuscatorData = {
            directExecute: true,
            configPath: configPath,
            arkGuardPath: arkGuardPath,
            debug: !!debug,
            buildConfig: buildConfig,
            logger: logger,
            obfuscatedOutputPath: obfuscatedOutputPath,
            mergedAbcFile: mergedAbcFile,
        };

        if (buildConfig.transformLib) {
            obfuscatorData.directExecute = false;
            obfuscatorData.logger = undefined;
            writeObfuscatorData(buildConfig.obfuscationOptions.obfuscationCacheDir, obfuscatorData);
            return true;
        }
        return executeObfuscator(obfuscatorData);
    }
}