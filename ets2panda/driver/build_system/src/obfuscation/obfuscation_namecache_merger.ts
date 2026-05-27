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
import { 
    BuildConfig,
    DeclFileNameCacheConfig,
    OHOS_MODULE_TYPE
} from '../types';
import { Logger, LogDataFactory } from '../logger';
import { ErrorCode } from '../util/error';
import { ensurePathExists } from '../util/utils';
import { BytecodeObfuscationConfig } from './obfuscation_bytecode_config';

const ALL_NAME_CACHE_FILE_NAME = '_$AllNameCache.json';
const NAME_CACHE_FILE_EXT = '.json';
const ETS_SUFFIX = '.ets';
const D_ETS_SUFFIX = '.d.ets';

function getNameCachePath(obfPath: string, dirName: string): string {
    const grandParentDir = path.dirname(path.dirname(obfPath));
    const nameCacheDir = path.join(grandParentDir, dirName);
    if (!fs.existsSync(nameCacheDir)) {
        fs.mkdirSync(nameCacheDir, { recursive: true });
    }
    return nameCacheDir;
}

/**
 * Extract the source-file stem (basename without .ets / .d.ets) used to match
 * the per-file name-cache JSON files produced by declgen.
 */
function getSourceFileStem(sourceFile: string): string {
    const base = path.basename(sourceFile);
    if (base.endsWith(D_ETS_SUFFIX)) {
        return base.slice(0, -D_ETS_SUFFIX.length);
    }
    if (base.endsWith(ETS_SUFFIX)) {
        return base.slice(0, -ETS_SUFFIX.length);
    }
    return base;
}

/**
 * Remove stale per-file name-cache JSON files that correspond to sources about
 * to be (re)compiled.
 */
function cleanStaleDeclNameCacheFiles(buildConfig: BuildConfig): void {
    const logger: Logger = Logger.getInstance();
    const nameCachePath = buildConfig.declFileNameCacheConfig?.nameCachePath;
    if (!nameCachePath || !fs.existsSync(nameCachePath)) {
        return;
    }
    const compileFiles = buildConfig.compileFiles;
    if (!compileFiles || compileFiles.length === 0) {
        return;
    }

    const stems = new Set<string>();
    for (const file of compileFiles) {
        const stem = getSourceFileStem(file);
        if (stem.length > 0) {
            stems.add(stem);
        }
    }
    if (stems.size === 0) {
        return;
    }

    let entries: string[];
    try {
        entries = fs.readdirSync(nameCachePath);
    } catch (error) {
        const errorMessage = error instanceof Error ? error.message : String(error);
        logger.printDebug(`Failed to read name cache directory ${nameCachePath}: ${errorMessage}`);
        return;
    }

    for (const entry of entries) {
        if (!entry.endsWith(NAME_CACHE_FILE_EXT) || entry === ALL_NAME_CACHE_FILE_NAME) {
            continue;
        }
        const entryStem = entry.slice(0, -NAME_CACHE_FILE_EXT.length);
        let matched = false;
        for (const target of stems) {
            if (entryStem === target || entryStem.endsWith('.' + target)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            continue;
        }
        const filePath = path.join(nameCachePath, entry);
        try {
            fs.unlinkSync(filePath);
            logger.printDebug(`Removed stale decl name cache file: ${filePath}`);
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : String(error);
            logger.printDebug(`Failed to remove stale decl name cache file ${filePath}: ${errorMessage}`);
        }
    }
}

export function genNameWhiteFile(buildConfig: BuildConfig): string{
    const mergedConfig = buildConfig.mergedObConfig;
    if (mergedConfig) {
        const bytecodeObfuscationConfig = new BytecodeObfuscationConfig(buildConfig, mergedConfig);
        buildConfig.bytecodeObfuscationConfig = bytecodeObfuscationConfig;
    }
    if (!buildConfig.bytecodeObfuscationConfig || buildConfig.bytecodeObfuscationConfig.obfuscationRules.disableObfuscation) {
        return '';
    }
    const obfuscatedOutputPath: string = buildConfig.bytecodeObfuscationConfig.obfAbcPath;
    const nameCachePath = getNameCachePath(obfuscatedOutputPath, 'namecache');
    const applyNameCacheDeclPath = path.join(nameCachePath, '_$AllNameCache.json');
    return applyNameCacheDeclPath;
}

export function initDeclFileNameCacheConfig(buildConfig: BuildConfig): void {
    const logger: Logger = Logger.getInstance();
    const mergedConfig = buildConfig.mergedObConfig;
    if (mergedConfig) {
        const bytecodeObfuscationConfig = new BytecodeObfuscationConfig(buildConfig, mergedConfig);
        buildConfig.bytecodeObfuscationConfig = bytecodeObfuscationConfig;
    }
    if (!buildConfig.bytecodeObfuscationConfig || buildConfig.bytecodeObfuscationConfig.obfuscationRules.disableObfuscation) {
        logger.printDebug('Obfuscation options are not enabled');
        return;
    }
    if ((buildConfig.moduleType !== OHOS_MODULE_TYPE.HAR) && (buildConfig.moduleType !== OHOS_MODULE_TYPE.SHARED)) {
        logger.printDebug('Hap has no declaration file.');
        return;
    }
    const obfuscatedOutputFile: string = buildConfig.bytecodeObfuscationConfig.obfAbcPath;
    const backupPath = getNameCachePath(obfuscatedOutputFile, 'namecache');
    const declFileNameCacheConfig: DeclFileNameCacheConfig = {
        nameCachePath: backupPath,
    };
    buildConfig.declFileNameCacheConfig = declFileNameCacheConfig;
    cleanStaleDeclNameCacheFiles(buildConfig);
}

export class JsonMerger {
    private buildConfig: BuildConfig;
    private logger: Logger;

    constructor(buildConfig: BuildConfig, logger: Logger) {
        this.buildConfig = buildConfig;
        this.logger = logger;
    }

    /**
     * Merge all JSON files in the specified directory into one JSON file
     */
    public mergeJsonFiles(): void {
        if (!this.buildConfig.declFileNameCacheConfig || !this.buildConfig.declFileNameCacheConfig.nameCachePath) {
            this.logger.printDebug('No name cache path configured');
            return;
        }
        const sourceDir = this.buildConfig.declFileNameCacheConfig.nameCachePath;
        if (!fs.existsSync(sourceDir)) {
            this.logger.printDebug(`JSON source directory does not exist: ${sourceDir}`);
            return;
        }
        const files = fs.readdirSync(sourceDir);
        const jsonFiles = files.filter((file: string) => file.endsWith('.json'));
        if (jsonFiles.length === 0) {
            this.logger.printDebug(`No JSON files found in directory: ${sourceDir}`);
            return;
        }
        const allNameCache = path.basename(genNameWhiteFile(this.buildConfig));
        this.logger.printDebug(`Found ${jsonFiles.length} JSON files, starting merge...`);
        const jsonObject: Record<string, string> = {};
        for (const jsonFile of jsonFiles) {
            if (jsonFile === allNameCache) {
                continue;
            }
            this.processJsonFile(sourceDir, jsonFile, jsonObject);
        }
        this.processNameCacheMerge(jsonObject);
    }

    /**
     * Process nameCache file merge logic
     */
    private processNameCacheMerge(mergedJsonObject: Record<string, string>): void {
        const bytecodeConfig = this.buildConfig.bytecodeObfuscationConfig;
        if (!bytecodeConfig || !bytecodeConfig.obfuscationRules.applyNameCacheDecl) {
            this.logger.printDebug('No nameCache file path found, skipping merge');
            return;
        }
        const targetPath = bytecodeConfig.obfuscationRules.applyNameCacheDecl;
        const generatedPath = genNameWhiteFile(this.buildConfig);
        const isGeneratedTarget = path.resolve(targetPath) === path.resolve(generatedPath);
        const existingObject = isGeneratedTarget ? {} : this.readExistingNameCacheFile(targetPath);
        const finalObject = this.mergeObjects(existingObject, mergedJsonObject);
        this.writeMergedResultToFile(finalObject);
    }

    /**
     * Read existing nameCache file if it exists
     */
    private readExistingNameCacheFile(targetPath: string): Record<string, string> {
        if (!fs.existsSync(targetPath)) {
            return {};
        }
        try {
            const content = fs.readFileSync(targetPath, 'utf-8');
            const existingObject = JSON.parse(content);
            return existingObject;
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : String(error);
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Failed to read applyNameCacheDecl file: ${targetPath}`,
                errorMessage
            ));
            return {};
        }
    }

    /**
     * Merge objects with overwrite strategy for duplicate keys
     */
    private mergeObjects(existingObject: Record<string, string>, mergedJsonObject: Record<string, string>): Record<string, string> {
        const finalObject: Record<string, string> = { ...existingObject };
        for (const [key, value] of Object.entries(mergedJsonObject)) {
            finalObject[key] = value;
            if (key in existingObject) {
                this.logger.printDebug(`Overwriting key "${key}" (from merged JSON files)`);
            }
        }
        return finalObject;
    }

    /**
     * Write final merged result to file
     */
    private writeMergedResultToFile(finalObject: Record<string, string>): void {
        if (!this.buildConfig.declFileNameCacheConfig?.nameCachePath) {
            return;
        }
        const filePath = genNameWhiteFile(this.buildConfig);
        try {
            ensurePathExists(filePath);
            fs.writeFileSync(filePath, JSON.stringify(finalObject));
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : String(error);
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Failed to write applyNameCacheDecl file: ${filePath}`,
                errorMessage
            ));
        }
    }

    /**
     * Process single JSON file and merge its content into target object
     */
    private processJsonFile(sourceDir: string, jsonFile: string, jsonObject: Record<string, string>): void {
        const filePath = path.join(sourceDir, jsonFile);
        try {
            const content = fs.readFileSync(filePath, 'utf-8');
            const jsonData = JSON.parse(content);
            if (typeof jsonData === 'object' && jsonData !== null) {
                for (const [key, value] of Object.entries<string>(jsonData)) {
                    jsonObject[key] = value;
                }
                this.logger.printDebug(`File read and merged: ${jsonFile}`);
            } else {
                this.logger.printError(LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                    `JSON file content is not an object: ${jsonFile}`,
                    'File content must be a JSON object for merging'
                ));
            }
        } catch (error) {
            const errorMessage = error instanceof Error ? error.message : String(error);
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Failed to read JSON file: ${jsonFile}`,
                errorMessage
            ));
        }
    }
}