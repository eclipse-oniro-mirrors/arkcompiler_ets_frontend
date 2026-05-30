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

import fs from 'fs';
import path from 'path';

import { BuildConfig, ObfuscationDependencies, ObfuscationOptions, OHOS_MODULE_TYPE } from '../types';
import { MergedConfig } from './obfuscation_merged_config';
import { LogData, Logger } from '../logger';
import { ErrorCode } from '../util/error';
import { containWildcards, ensurePathExists, getAbsPathBaseConfigPath, resolvePath, sortAndDeduplicateStringArr, toUnixPath } from '../util/utils';
import { collectSdkApiWhitelist, getSdkApiList } from './obfuscation_collect_whitelist';
export enum OptionType {
    NONE,
    KEEP,
    KEEP_CLASS_WITH_MEMBERS,
    KEEP_MEMBERS,
    PRINT_CONFIGURATION,
    PRINT_SEEDS,
    DISABLE_OBFUSCATION,
    KEEP_FILE_NAME,
    PRINT_NAMECACHE,
    APPLY_NAMECACHE,
    APPLY_NAMECACHE_DECL,
    REMOVE_LOG,
    ENABLE_BYTECODE_OBFUSCATION,
    ENABLE_BYTECODE_OBFUSCATION_DEBUGGING
}

interface ObfuscationConfigToken {
    directive: string;
    args: string[] | boolean;
    type: OptionType;
}

export class ObConfigResolver {
    static readonly DISABLE_OBFUSCATION = '-disable-obfuscation';
    static readonly PRINT_CONFIGURATION = '-print-configuration';
    static readonly KEEP = '-keep';
    static readonly KEEP_CLASS_WITH_MEMBERS = '-keep-class-with-members';
    static readonly KEEP_CLASS_MEMBERS = '-keep-class-members';
    static readonly PRINT_SEEDS = '-print-seeds';
    static readonly KEEP_FILE_NAME = '-keep-file-name';
    static readonly PRINT_NAMECACHE = '-print-namecache';
    static readonly APPLY_NAMECACHE = '-apply-namecache';
    static readonly APPLY_NAMECACHE_DECL = '-apply-namecache-decl';
    static readonly REMOVE_LOG = '-remove-log';
    static readonly ENABLE_BYTECODE_OBFUSCATION = '-enable-bytecode-obfuscation';
    static readonly ENABLE_BYTECODE_OBFUSCATION_DEBUGGING = '-enable-bytecode-obfuscation-debugging';
    static readonly KEEP_CLASS_WITH_MEMBERS_ARKGUARD = '-keepclasswithmembers';
    static readonly KEEP_CLASS_MEMBERS_ARKGUARD = '-keepclassmembers';

    obfuscationOptions: ObfuscationOptions | undefined;
    isHarCompiled: boolean | undefined;
    isHspCompiled: boolean | undefined;
    needConsumerConfigs: boolean = false;
    dependencyConfigs: MergedConfig | undefined;
    configPath: string | undefined;

    constructor(private buildConfig: BuildConfig) {
        this.obfuscationOptions = buildConfig.obfuscationOptions;
        if (this.obfuscationOptions && this.obfuscationOptions.sdkApis.length === 0) {
            buildConfig.obfuscationOptions!.sdkApis = getSdkApiList(buildConfig);
        }
        this.isHarCompiled = buildConfig.moduleType === OHOS_MODULE_TYPE.HAR;
        this.isHspCompiled = buildConfig.moduleType === OHOS_MODULE_TYPE.SHARED;
    }
    private static readonly TOKEN_TYPE_MAP: Map<string, OptionType> = new Map([
        [ObConfigResolver.DISABLE_OBFUSCATION, OptionType.DISABLE_OBFUSCATION],
        [ObConfigResolver.KEEP, OptionType.KEEP],
        [ObConfigResolver.KEEP_CLASS_WITH_MEMBERS, OptionType.KEEP_CLASS_WITH_MEMBERS],
        [ObConfigResolver.KEEP_CLASS_MEMBERS, OptionType.KEEP_MEMBERS],
        [ObConfigResolver.PRINT_CONFIGURATION, OptionType.PRINT_CONFIGURATION],
        [ObConfigResolver.PRINT_SEEDS, OptionType.PRINT_SEEDS],
        [ObConfigResolver.KEEP_FILE_NAME, OptionType.KEEP_FILE_NAME],
        [ObConfigResolver.PRINT_NAMECACHE, OptionType.PRINT_NAMECACHE],
        [ObConfigResolver.APPLY_NAMECACHE, OptionType.APPLY_NAMECACHE],
        [ObConfigResolver.APPLY_NAMECACHE_DECL, OptionType.APPLY_NAMECACHE_DECL],
        [ObConfigResolver.REMOVE_LOG, OptionType.REMOVE_LOG],
        [ObConfigResolver.ENABLE_BYTECODE_OBFUSCATION, OptionType.ENABLE_BYTECODE_OBFUSCATION],
        [ObConfigResolver.ENABLE_BYTECODE_OBFUSCATION_DEBUGGING, OptionType.ENABLE_BYTECODE_OBFUSCATION_DEBUGGING],
    ]);

    static readonly exportedSwitchMap: Map<string, string> = new Map([
        ['removeLog', ObConfigResolver.REMOVE_LOG],
    ]);

    resolveObfuscationConfigs(): MergedConfig {
        const obfConfig = this.obfuscationOptions;

        if (!this.hasValidSource(obfConfig)) {
            return new MergedConfig();
        }

        let enable = obfConfig?.selfConfig?.ruleOptions?.enable;
        const selfConfigs = this.buildSelfConfigs(enable);

        enable = this.updateEnableBySelfConfigs(enable, selfConfigs);

        this.needConsumerConfigs = this.computeNeedConsumerConfigs(obfConfig);

        const { config: newDependencyConfigs, enable: updatedEnable } = this.buildDependencyConfigs(obfConfig, enable);

        enable = updatedEnable;
        this.dependencyConfigs = newDependencyConfigs;

        const mergedConfigs = this.getMergedConfigs(selfConfigs, this.dependencyConfigs);
        this.handleSystemApiCache(enable, obfConfig, mergedConfigs);
        this.emitConsumerConfigsIfNeeded(enable);

        return mergedConfigs;
    }

    private hasValidSource(obfConfig: ObfuscationOptions | undefined): boolean {
        return !!(obfConfig && obfConfig.selfConfig && obfConfig.selfConfig.ruleOptions);
    }

    private buildSelfConfigs(enable?: boolean): MergedConfig {
        const selfConfigs = new MergedConfig();

        if (!enable) {
            selfConfigs.options.disableObfuscation = true;
            return selfConfigs;
        }

        const rules = this.obfuscationOptions?.selfConfig?.ruleOptions?.rules;
        if (rules) {
            for (const path of rules) {
                this.getConfigByPath(path, selfConfigs);
            }
        }

        return selfConfigs;
    }

    private updateEnableBySelfConfigs(enable: boolean | undefined, configs: MergedConfig): boolean {
        if (enable === undefined) {
            return false;
        }
        return enable && !configs.options.disableObfuscation;
    }

    private computeNeedConsumerConfigs(obfConfig: ObfuscationOptions | undefined): boolean {
        return !!(this.isHarCompiled || this.isHspCompiled) && 
               !!obfConfig?.selfConfig?.consumerRules && 
               obfConfig.selfConfig.consumerRules.length > 0;
    }

    private buildDependencyConfigs(obfConfig: ObfuscationOptions | undefined, enable: boolean): { config: MergedConfig, enable: boolean } {
        const dependencyConfigs = new MergedConfig();

        const dependencies = obfConfig?.dependencies;
        const maxLen = Math.max(
            dependencies?.libraries?.length ?? 0,
            dependencies?.hars?.length ?? 0,
            dependencies?.hsps?.length ?? 0,
            dependencies?.hspLibraries?.length ?? 0
        );

        if ((enable || this.needConsumerConfigs) && maxLen > 0 && dependencies) {
            this.getDependencyConfigs(dependencies, dependencyConfigs);
            enable = enable && !dependencyConfigs.options.disableObfuscation;
        }

        return { config: dependencyConfigs, enable }
    }

    private handleSystemApiCache(enable: boolean, obfConfig: ObfuscationOptions | undefined, merged: MergedConfig): void {
        if (!enable || !obfConfig?.obfuscationCacheDir) {
            return;
        }

        const systemApiCachePath = path.join(obfConfig.obfuscationCacheDir, 'systemApiCache.txt');
        if (!fs.existsSync(systemApiCachePath)) {
            this.collectSystemApiWhitelist(merged, systemApiCachePath);
        }
    }

    private emitConsumerConfigsIfNeeded(enable: boolean): void {
        if (!enable) {
            this.emitConsumerConfigFiles();
        }
    }

    private collectSystemApiWhitelist(mergedConfigs: MergedConfig, systemApiCachePath: string): void {
        const sdkApis: string[] = sortAndDeduplicateStringArr(this.obfuscationOptions?.sdkApis || []);
        const outputFilePath = systemApiCachePath;
        ensurePathExists(outputFilePath);
        for (let apiPath of sdkApis) {
            let res = collectSdkApiWhitelist(apiPath);
            let allContent = res.replace(/^\n+/, '').replace(/\n\s*\n+/g, '\n\n');
            fs.appendFileSync(outputFilePath, allContent, 'utf8');
        }
    }

    public emitConsumerConfigFiles(): void {
        if (this.needConsumerConfigs && this.obfuscationOptions && this.dependencyConfigs) {
            let selfConsumerConfig = new MergedConfig();
            this.getSelfConsumerConfig(selfConsumerConfig);
            this.genConsumerConfigFiles(this.obfuscationOptions, selfConsumerConfig, this.dependencyConfigs);
        }
    }

    private genConsumerConfigFiles(options: ObfuscationOptions, selfConsumerConfig: MergedConfig, dependencyConfigs: MergedConfig): void {
        if (this.isHarCompiled) {
            selfConsumerConfig.mergeAllRules(dependencyConfigs);
        }
        selfConsumerConfig.sortAndDeduplicate();
        this.writeConsumerConfigFile(selfConsumerConfig, options.exportRulePath);
    }

    public writeConsumerConfigFile(selfConsumerConfig: MergedConfig, outpath: string): void {
        if (!outpath) {
            return;
        }
        const configContent: string = selfConsumerConfig.serializeMergedConfig();
        try {
            ensurePathExists(outpath);
            fs.writeFileSync(outpath, configContent);
        } catch (err) {
            const logger: Logger = Logger.getInstance();
            const errorCodeInfo: LogData = {
                code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                description: 'ArkTS compiler Error',
                cause: `Failed to write consumer config file to ${outpath}.`,
                position: outpath,
                solutions: [`Please check whether you have write permission to ${outpath}.`],
            };
            logger.printError(errorCodeInfo);
        }
    }

    private getSelfConsumerConfig(selfConsumerConfig: MergedConfig): void {
        for (const path of this.obfuscationOptions?.selfConfig?.consumerRules || []) {
            this.getConfigByPath(path, selfConsumerConfig);
        }
    }

    private getConfigByPath(path: string, configs: MergedConfig): void {
        let fileContent = undefined;
        try {
            fileContent = fs.readFileSync(path, 'utf-8');
        } catch (err) {
            const logger: Logger = Logger.getInstance();
            const errorCodeInfo: LogData = {
                code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                description: 'ArkTS compiler Error',
                cause: `Failed to open obfuscation config file from ${path}. Error message: ${err}`,
                position: path,
                solutions: [`Please check whether ${path} exists.`],
            };
            logger.printError(errorCodeInfo);
        }
        if (!fileContent) {
            return;
        }
        this.handleConfigContent(fileContent, configs, path);
    }

    private removeComments(data: string): string {
        const commentStart = '#';
        const commentEnd = '\n';
        let cleanedString = '';
        let isInComments = false;

        for (let i = 0; i < data.length; i++) {
            if (isInComments) {
                isInComments = data[i] !== commentEnd;
            } else if (data[i] !== commentStart) {
                cleanedString += data[i];
            } else {
                isInComments = true;
            }
        }
        return cleanedString;
    }

    private getTokenType(token: string): OptionType {
        return ObConfigResolver.TOKEN_TYPE_MAP.get(token) || OptionType.NONE;
    }

    private mergeDependencyConfigsByPath(paths: string[], dependencyConfigs: MergedConfig): void {
        for (const path of paths) {
            const tmpConfig = new MergedConfig();
            this.getConfigByPath(path, tmpConfig);
            dependencyConfigs.mergeAllRules(tmpConfig);
        }
    }

    private getDependencyConfigs(dependencies: ObfuscationDependencies, dependencyConfigs: MergedConfig): void {
        for (const lib of dependencies.libraries || []) {
            if (lib.consumerRules && lib.consumerRules.length > 0) {
                this.mergeDependencyConfigsByPath(lib.consumerRules, dependencyConfigs);
            }
        }
        for (const lib of dependencies.hspLibraries || []) {
            if (lib.consumerRules && lib.consumerRules.length > 0) {
                this.mergeDependencyConfigsByPath(lib.consumerRules, dependencyConfigs);
            }
        }
        if (dependencies && dependencies.hars && dependencies.hars.length > 0) {
            this.mergeDependencyConfigsByPath(dependencies.hars, dependencyConfigs);
        }
        if (dependencies && dependencies.hsps && dependencies.hsps.length > 0) {
            this.mergeDependencyConfigsByPath(dependencies.hsps, dependencyConfigs);
        }
    }

    isClassSpecStart(line: string): boolean {
        const trimmed = line.trim();
        if (trimmed === '') {
            return false;
        }
        return /^(@\S+\s+)?([\w!]+\s+)*(class|interface|enum|record|namespace)\b/.test(trimmed);
    }

    private trimTrailingEmpty(lines: string[]): void {
        while (
            lines.length > 0 &&
            lines[lines.length - 1].trim() === ''
        ) {
            lines.pop();
        }
    }

    splitClassSpecBlocks(rawBlock: string): string[] {
        const lines = rawBlock.split('\n');
        const blocks: string[] = [];
        let currentBlockLines: string[] = [];
        let insideBraces = 0;

        const pushCurrentBlock = (): void => {
            this.trimTrailingEmpty(currentBlockLines);
            if (currentBlockLines.length > 0) {
                blocks.push(currentBlockLines.join('\n'));
            }
            currentBlockLines = [];
        };
        
        for (const line of lines) {
            const trimmed = line.trim();
            if (insideBraces === 0 && this.isClassSpecStart(trimmed)) {
                if (currentBlockLines.length > 0) {
                    pushCurrentBlock();
                }
            }
            currentBlockLines.push(line);
            for (const char of line) {
                if (char === '{') {
                    insideBraces++;
                } else if (char === '}') {
                    insideBraces--;
                }
            }
        }

        pushCurrentBlock();
        return blocks;
    }

    parseObfuscationConfig(configText: string): ObfuscationConfigToken[] {
        const lines = configText.split(/\r?\n/);
        const result: ObfuscationConfigToken[] = [];
        let index = 0;

        while (index < lines.length) {
            let line = lines[index].trim();
            if (!line.startsWith('-')) {
                index++;
                continue;
            }
            const { directive, inlinePart, type } = this.parseDirective(line);
            if (this.isSimpleBooleanOption(type)) {
                index = this.handleBooleanOption(result, index, directive, type);
            } else if (this.isKeepOption(type)) {
                index = this.handleKeepOption(result, index, lines, directive, inlinePart, type);
            } else {
                index = this.handleNormalOption(result, index, lines, directive, inlinePart, type);
            }
        }
        return result;
    }

    private parseDirective(line: string): { directive: string, inlinePart: string | null, type: OptionType } {
        const firstSpaceIndex = line.indexOf(' ');
        let directive: string;
        let inlinePart: string | null = null;

        if (firstSpaceIndex === -1) {
            directive = line;
        } else {
            directive = line.substring(0, firstSpaceIndex);
            inlinePart = line.substring(firstSpaceIndex + 1);
        }

        const type = this.getTokenType(directive);
        return { directive, inlinePart, type };
    }

    private handleBooleanOption(result: ObfuscationConfigToken[], index: number, directive: string, type: OptionType): number {
        result.push({ directive, args: true, type });
        return index + 1;
    }

    private handleKeepOption(
        result: ObfuscationConfigToken[],
        index: number,
        lines: string[],
        directive: string,
        inlinePart: string | null,
        tokenType: OptionType
    ): number {
        const blockLines: string[] = [];
        if (inlinePart !== null && inlinePart !== '') {
            blockLines.push(inlinePart);
        }
        index++;
        while (index < lines.length) {
            const currentRawLine = lines[index];
            const trimmedNext = currentRawLine.trim();
            if (trimmedNext.startsWith('-') && !trimmedNext.startsWith('--')) {
                break;
            }
            blockLines.push(currentRawLine);
            index++;
        }
        const rawBlock = blockLines.join('\n');
        if (this.isEntireBlockPaths(rawBlock)) {
            const pathLines = blockLines.map(l => l.trim())
                .filter(l => l !== '');
            if (pathLines.length > 0) {
                result.push({
                    directive: '-keep',
                    args: pathLines,
                    type: tokenType
                });
            }
        } else {
            const classSpecBlocks = this.splitClassSpecBlocks(rawBlock);
            for (const block of classSpecBlocks) {
                if (block.trim() === '') {
                    continue;
                }
                result.push({
                    directive,
                    args: [block],
                    type: tokenType
                });
            }
        }
        return index;
    }

    private handleNormalOption(
        result: ObfuscationConfigToken[],
        index: number,
        lines: string[],
        directive: string,
        inlinePart: string | null,
        type: OptionType
    ): number {
        const argsList: string[] = [];
        if (inlinePart !== null && inlinePart.trim() !== '') {
            argsList.push(...inlinePart.trim().split(' '));
        }
        index++;
        while (index < lines.length) {
            const nextLine = lines[index].trim();
            if (nextLine.startsWith('-')) {
                break;
            }
            if (nextLine !== '') {
                argsList.push(...nextLine.trim().split(' '));
            }
            index++;
        }
        result.push({ directive, args: argsList, type });
        return index;
    }

    private handleConfigContent(data: string, configs: MergedConfig, configPath: string): void {
        const cleanedData = this.removeComments(data);
        const tokens: ObfuscationConfigToken[] = this.parseObfuscationConfig(cleanedData);
        const configHandlers: Record<OptionType, (token: ObfuscationConfigToken) => void> = {
            [OptionType.ENABLE_BYTECODE_OBFUSCATION]: () => {
                this.handleEnableBytecodeObfuscation(configs);
            },
            [OptionType.ENABLE_BYTECODE_OBFUSCATION_DEBUGGING]: () => {
                this.handleEnableBytecodeObfuscationDebugging(configs);
            },
            [OptionType.DISABLE_OBFUSCATION]: () => {
                this.handleDisableObfuscation(configs);
            },
            [OptionType.PRINT_CONFIGURATION]: (token) => {
                this.handlePrintConfiguration(token, configs, configPath);
            },
            [OptionType.KEEP]: (token) => {
                this.handleKeep(token, configs, configPath);
            },
            [OptionType.KEEP_CLASS_WITH_MEMBERS]: (token) => {
                this.handleKeepClassWithMembers(token, configs);
            },
            [OptionType.KEEP_MEMBERS]: (token) => {
                this.handleKeepMembers(token, configs);
            },
            [OptionType.PRINT_SEEDS]: (token) => {
                this.handlePrintSeeds(token, configs, configPath);
            },
            [OptionType.KEEP_FILE_NAME]: (token) => {
                this.handleKeepFileName(token, configs);
            },
            [OptionType.PRINT_NAMECACHE]: (token) => {
                this.handlePrintNamecache(token, configs, configPath);
            },
            [OptionType.APPLY_NAMECACHE]: (token) => {
                this.handleApplyNamecache(token, configs, configPath);
            },
            [OptionType.APPLY_NAMECACHE_DECL]: (token) => {
                this.handleApplyNamecacheDecl(token, configs, configPath);
            },
            [OptionType.REMOVE_LOG]: () => {
                this.handleRemoveLog(configs);
            },
            [OptionType.NONE]: () => { },
        };
        for (const token of tokens) {
            const handler = configHandlers[token.type] || ((_token: ObfuscationConfigToken): void => { });
            handler(token);
        }
    }

    private handleEnableBytecodeObfuscation(configs: MergedConfig): void {
        configs.options.bytecodeObf.enable = true;
        configs.options.bytecodeObf.configPath = path.join(
            this.buildConfig!.obfuscationOptions!.obfuscationCacheDir,
            'config.json'
        );
    }

    private handleEnableBytecodeObfuscationDebugging(configs: MergedConfig): void {
        configs.options.bytecodeObf.debugging = true;
    }

    private handleDisableObfuscation(configs: MergedConfig): void {
        configs.options.disableObfuscation = true;
    }

    private handlePrintConfiguration(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        if (Array.isArray(token.args)) {
            if (token.args.length > 0) {
                configs.options.printConfiguration = resolvePath(configPath, token.args[0]);
            } else {
                const errorCodeInfo: LogData = {
                    code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                    description: 'ArkTS compiler Error',
                    cause: `"${ObConfigResolver.PRINT_CONFIGURATION}" requires a file path argument`,
                    position: configPath,
                    solutions: [`Add the file path argument after "${ObConfigResolver.PRINT_CONFIGURATION}" in ${configPath}`],
                };
                Logger.getInstance().printError(errorCodeInfo);
            }
        }
    }

    private handleKeep(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        if (!Array.isArray(token.args) || token.args.length === 0) {
            return;
        }
        configs.options.enableKeep = true;
        const [keepPaths, keeps] = token.args.reduce(
            ([paths, names], arg) => {
                if (this.isLikelyPathLine(arg)) {
                    paths.push(arg);
                } else {
                    names.push(arg);
                }
                return [paths, names];
            },
            [[] as string[], [] as string[]]
        );

        if (keepPaths.length > 0) {
            this.resolveKeepConfig(keepPaths, configs);
        }

        if (keeps.length > 0) {
            const keepItems = this.appItem(ObConfigResolver.KEEP, keeps);
            configs.keepClassSpecLists.push(...keepItems);
        }
    }

    private handleKeepClassWithMembers(token: ObfuscationConfigToken, configs: MergedConfig): void {
        configs.options.enableKeepClassWithMembers = true;
        if (Array.isArray(token.args)) {
            const keepItems = this.appItem(ObConfigResolver.KEEP_CLASS_WITH_MEMBERS_ARKGUARD, token.args);
            configs.keepClassWithMembers.push(...keepItems);
        }
    }

    private handleKeepMembers(token: ObfuscationConfigToken, configs: MergedConfig): void {
        configs.options.enableKeepMembers = true;
        if (Array.isArray(token.args)) {
            const keepItems = this.appItem(ObConfigResolver.KEEP_CLASS_MEMBERS_ARKGUARD, token.args);
            configs.keepMembers.push(...keepItems);
        }
    }

    private handlePrintSeeds(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        configs.options.enablePrintSeeds = true;
        if (Array.isArray(token.args) && token.args.length > 0) {
            configs.options.printSeedsFilePath = resolvePath(configPath, token.args[0]);
        } else {
            configs.options.printSeedsFilePath = path.join(
                this.buildConfig!.obfuscationOptions!.obfuscationCacheDir,
                'seedsFile.log'
            );
        }   
    }

    private handleKeepFileName(token: ObfuscationConfigToken, configs: MergedConfig): void {
        if (Array.isArray(token.args) && token.args.length > 0) {
            configs.reservedFileNames.push(...token.args);
        }
    }

    private handlePrintNamecache(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        if (Array.isArray(token.args) && token.args.length > 0) {
            configs.options.printNameCache = resolvePath(configPath, token.args[0]);
        } else {
            const errorCodeInfo: LogData = {
                code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                description: 'ArkTS compiler Error',
                cause: `"${ObConfigResolver.PRINT_NAMECACHE}" requires a file path argument`,
                position: configPath,
                solutions: [`Add the file path argument after "${ObConfigResolver.PRINT_NAMECACHE}" in ${configPath}`],
            };
            Logger.getInstance().printError(errorCodeInfo);
        }
    }

    private handleApplyNamecache(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        if (Array.isArray(token.args) && token.args.length > 0) {
            const absNameCachePath: string = resolvePath(configPath, token.args[0]);
            ensurePathExists(absNameCachePath);
            configs.options.applyNameCache = absNameCachePath;
        } else {
            const errorCodeInfo: LogData = {
                code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                description: 'ArkTS compiler Error',
                cause: `"${ObConfigResolver.APPLY_NAMECACHE}" requires a file path argument`,
                position: configPath,
                solutions: [`Add the file path argument after "${ObConfigResolver.APPLY_NAMECACHE}" in ${configPath}`],
            };
            Logger.getInstance().printError(errorCodeInfo);
        }
    }

    private handleApplyNamecacheDecl(token: ObfuscationConfigToken, configs: MergedConfig, configPath: string): void {
        if (Array.isArray(token.args) && token.args.length > 0) {
            const absNameCacheDeclPath: string = resolvePath(configPath, token.args[0]);
            ensurePathExists(absNameCacheDeclPath);
            configs.options.applyNameCacheDecl = absNameCacheDeclPath;
        } else {
            const errorCodeInfo: LogData = {
                code: ErrorCode.BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR,
                description: 'ArkTS compiler Error',
                cause: `"${ObConfigResolver.APPLY_NAMECACHE_DECL}" requires a file path argument`,
                position: configPath,
                solutions: [`Add the file path argument after "${ObConfigResolver.APPLY_NAMECACHE_DECL}" in ${configPath}`],
            };
            Logger.getInstance().printError(errorCodeInfo);
        }
    }

    private handleRemoveLog(configs: MergedConfig): void {
        configs.options.removeLog = true;
    }

    private appItem(prefix: string, items: string[]): string[] {
        let keepItems: string[] = [];
        items.forEach(item => {
            keepItems.push(prefix + ' ' + item);
        })
        return keepItems;
    }

    isEntireBlockPaths(rawBlock: string): boolean {
        const lines = rawBlock.split('\n');
        for (const line of lines) {
            if (line.trim() === '') {
                continue;
            }
            if (!this.isLikelyPathLine(line)) {
                return false;
            }
        }
        return true;
    }

    isLikelyPathLine(line: string): boolean {
        const trimmed = line.trim();
        if (trimmed === '') {
            return true;
        }
        if (
            trimmed.includes('{') ||
            trimmed.includes('}') ||
            trimmed.includes('(') ||
            /\b(extends|implements|class|interface|enum|record)\b/.test(trimmed)
        ) {
            return false;
        }
        const withoutExclamation = trimmed.startsWith('!') ? trimmed.slice(1).trim() : trimmed;
        return /[\/\\.*]/.test(withoutExclamation) || withoutExclamation === '';

    }
    public resolveKeepConfig(keepConfigs: string[], configs: MergedConfig): void {
        for (let keepPath of keepConfigs) {
            let tempAbsPath : string = keepPath;
            if (containWildcards(tempAbsPath)) {
                configs.keepUniversalPaths.push(toUnixPath(tempAbsPath));
                continue;
            }
            configs.keepSourceOfPaths.push(toUnixPath(tempAbsPath));
        }
    }

    private isSimpleBooleanOption(tokenType: OptionType): boolean {
        return [
            OptionType.DISABLE_OBFUSCATION,
            OptionType.REMOVE_LOG,
        ].includes(tokenType);
    }

    private isKeepOption(tokenType: OptionType): boolean {
        return [
            OptionType.KEEP,
            OptionType.KEEP_CLASS_WITH_MEMBERS,
            OptionType.KEEP_MEMBERS,
        ].includes(tokenType);
    }

    private getMergedConfigs(selfConfigs: MergedConfig, dependencyConfigs: MergedConfig): MergedConfig {
        if (dependencyConfigs) {
            selfConfigs.mergeKeepOptions(dependencyConfigs);
        }
        selfConfigs.sortAndDeduplicate();
        return selfConfigs;
    }
}
