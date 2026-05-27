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

import path from 'path';
import fs from 'fs';

import { BuildConfig, Obfuscation, BUILD_MODE } from '../types';
import { Logger, LogDataFactory } from '../logger';
import { ErrorCode } from '../util/error';
import { ObConfigResolver } from './obfuscation_config_resolver';
import { ensurePathExists } from '../util/utils';
import { BytecodeObfuscationConfig } from './obfuscation_bytecode_config';
import { obfuscation_default_keep, forEachKeepRule, KeepRuleType, RuleTargetType, KeepRule } from './obfuscation_default_keep';
import { MergedConfig } from './obfuscation_merged_config';
import { getDefaultKeepFileNameWhitelist } from './obfuscation_collect_whitelist';
import { genNameWhiteFile } from '../obfuscation/obfuscation_namecache_merger';

export function initObfuscationConfig(buildConfig: BuildConfig): void {
    const logger: Logger = Logger.getInstance();
    validatePObfuscationOptions(buildConfig);
    logger.printDebug('Initializing obfuscation config...');
    const isDebug = buildConfig.buildMode === BUILD_MODE.DEBUG;
    if (!buildConfig.obfuscationOptions || isDebug) {
        logger.printDebug('Obfuscation options are not enabled in debug mode, skipping obfuscation config initialization');
        return;
    }
    try {
        const obConfigResolver: ObConfigResolver = new ObConfigResolver(buildConfig);
        buildConfig.mergedObConfig = obConfigResolver.resolveObfuscationConfigs();
        buildConfig.mergedObConfig.options.applyNameCacheDecl = genNameWhiteFile(buildConfig);
        if (buildConfig.mergedObConfig?.options?.printConfiguration !== undefined) {
            const configOutput = buildConfig.mergedObConfig.printMergedConfig();
            let outputPath = buildConfig.mergedObConfig.options.printConfiguration;
            if (outputPath === '') {
                outputPath = path.join(buildConfig.obfuscationOptions!.obfuscationCacheDir, 'printConfiguration.txt');
            }
            const dirName = path.dirname(outputPath);
            if (!fs.existsSync(dirName)) {
                fs.mkdirSync(dirName, { recursive: true });
            }
            fs.writeFileSync(outputPath, configOutput);
            logger.printInfo(`Obfuscation merged configuration written to: ${outputPath}`);
        }
        logger.printDebug('Obfuscation config initialized successfully.');
    } catch (error) {
        const errorMessage = `Failed to initialize obfuscation config: ${error instanceof Error ? error.message : String(error)}`;
        logger.printError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL, errorMessage));
    }
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

function validatePObfuscationOptions(projectConfig: BuildConfig): void {
    if (projectConfig.projectRootPath) {
        projectConfig.projectRootPath = getVar(projectConfig.projectRootPath);
    }
    const obfOpts = projectConfig.obfuscationOptions;
    if (!obfOpts) {
        return;
    }
    obfOpts.obfuscationCacheDir = getVar(obfOpts.obfuscationCacheDir);
    obfOpts.exportRulePath = getVar(obfOpts.exportRulePath);
    processSelfConfig(obfOpts.selfConfig);
}

function processSelfConfig(selfConfig?: Obfuscation): void {
    if (!selfConfig) {
        return;
    }
    const { ruleOptions, consumerRules } = selfConfig;
    if (ruleOptions?.rules) {
        ruleOptions.rules = ruleOptions.rules.map(getVar);
    }
    if (consumerRules) {
        selfConfig.consumerRules = consumerRules.map(getVar);
    }
}

export function genObfuscationConfig(buildConfig: BuildConfig, entryFiles: Set<string>): void {
    const mergedObConfig = buildConfig.mergedObConfig;
    if (!mergedObConfig) {
        return;
    }
    const isObfEnabled = mergedObConfig.options.bytecodeObf.enable;
    if (!isObfEnabled) {
        return;
    }
    try {
        processDefaultObfuscationRules(buildConfig, mergedObConfig, entryFiles);
        const obfuscationCacheDir = buildConfig.obfuscationOptions?.obfuscationCacheDir;
        if (!obfuscationCacheDir) {
            throw new Error('Obfuscation cache directory is not configured');
        }
        const obfDir = path.join(obfuscationCacheDir, 'obf');
        if (!fs.existsSync(obfDir)) {
            fs.mkdirSync(obfDir, { recursive: true });
        }
        mergedObConfig.options.bytecodeObf.enable = !mergedObConfig.options.disableObfuscation;
        mergedObConfig.reservedFileNames.push(...getDefaultKeepFileNameWhitelist(buildConfig));
        const bytecodeObfuscationConfig = new BytecodeObfuscationConfig(buildConfig, mergedObConfig);
        buildConfig.bytecodeObfuscationConfig = bytecodeObfuscationConfig;
        const configPath = mergedObConfig.options.bytecodeObf.configPath;
        ensurePathExists(configPath);
        fs.writeFileSync(configPath, JSON.stringify(bytecodeObfuscationConfig, (key, value) => {
            if (value instanceof Set) {
                return Array.from(value);
            }
            return value;
        }, 2));
    } catch (error) {
        const logger: Logger = Logger.getInstance();
        const errorMessage = `Failed to generate obfuscation config: ${error instanceof Error ? error.message : String(error)}`;
        logger.printError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_GEN_OBFUSCATION_CONFIG_FAIL, errorMessage));
    }
}

export enum TargetType {
    KEEP_CLASS_SPEC_LISTS = 'keepClassSpecLists',
    KEEP_MEMBERS = 'keepMembers',
    KEEP_CLASS_WITH_MEMBERS = 'keepClassWithMembers'
}

function processDefaultObfuscationRules(
    buildConfig: BuildConfig,
    mergedObConfig: MergedConfig,
    entryFiles: Set<string>
): void {
    const ruleSettings: Record<string, { prefix: string; target: TargetType }> = {
        [KeepRuleType.KEEP]: { prefix: ObConfigResolver.KEEP, target: TargetType.KEEP_CLASS_SPEC_LISTS },
        [KeepRuleType.KEEP_CLASS_WITH_MEMBERS]: { prefix: ObConfigResolver.KEEP_CLASS_WITH_MEMBERS_ARKGUARD, target: TargetType.KEEP_CLASS_WITH_MEMBERS },
        [KeepRuleType.KEEP_CLASS_MEMBERS]: { prefix: ObConfigResolver.KEEP_CLASS_MEMBERS_ARKGUARD, target: TargetType.KEEP_MEMBERS }
    };
    forEachKeepRule(obfuscation_default_keep, (rules: KeepRule[], ruleType: KeepRuleType) => {
        if (ruleType === KeepRuleType.APPEND_ALL_FILE) {
            processAppendAllFileRules(mergedObConfig, entryFiles, rules);
            return;
        }
        const setting = ruleSettings[ruleType];
        if (!setting) {
            return; 
        } 
        rules.forEach((rule: { value: string, type: RuleTargetType }) => {
            const fullRule = `${setting.prefix} ${rule.type} ${rule.value}`;
            if (setting.target === TargetType.KEEP_CLASS_SPEC_LISTS) {
                mergedObConfig.keepClassSpecLists.push(fullRule);
            } else if (setting.target === TargetType.KEEP_MEMBERS) {
                mergedObConfig.keepMembers.push(fullRule);
            } else if (setting.target === TargetType.KEEP_CLASS_WITH_MEMBERS) {
                mergedObConfig.keepClassWithMembers.push(fullRule);
            }
        });
    });
}

function processAppendAllFileRules(
    mergedObConfig: MergedConfig,
    entryFiles: Set<string>,
    rules: { value: string, type: RuleTargetType }[]
): void {
    if (rules.length === 0 || entryFiles.size === 0) {
        return;
    }
    rules.forEach((rule: { value: string, type: RuleTargetType }) => {
        const fullRule = `${ObConfigResolver.KEEP} ${rule.type} ${rule.value}`;
        mergedObConfig.keepClassSpecLists.push(fullRule);
    });
}
