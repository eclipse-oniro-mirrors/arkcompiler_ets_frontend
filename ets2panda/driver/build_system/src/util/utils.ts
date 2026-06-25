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

import * as crypto from 'crypto';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

import {
    ARKTS_MODULE_NAME,
    DECL_ETS_SUFFIX,
    ETSCACHE_SUFFIX,
    LANGUAGE_VERSION,
    NATIVE_MODULE,
    sdkConfigPrefix,
    MAX_PATH_LENGTH,
    TS_SUFFIX
} from '../pre_define';
import {
    Logger,
    LogDataFactory
} from '../logger';
import { ErrorCode, DriverError } from '../util/error';
import {
    ModuleInfo,
    OHOS_MODULE_TYPE,
    BuildConfig,
    DependencyModuleConfig,
} from '../types';

const WINDOWS: string = 'Windows_NT';
const LINUX: string = 'Linux';
const MAC: string = 'Darwin';

export function isWindows(): boolean {
    return os.type() === WINDOWS;
}

export function isLinux(): boolean {
    return os.type() === LINUX;
}

export function isMac(): boolean {
    return os.type() === MAC;
}

export function changeFileExtension(file: string, targetExt: string, originExt = ''): string {
    let currentExt = originExt.length === 0 ? getFileExtension(file) : originExt;
    let fileWithoutExt = file.substring(0, file.lastIndexOf(currentExt));
    return fileWithoutExt + targetExt;
}

export function changeDeclgenFileExtension(file: string, targetExt: string): string {
    if (file.endsWith(DECL_ETS_SUFFIX)) {
        return changeFileExtension(file, targetExt, DECL_ETS_SUFFIX);
    }
    return changeFileExtension(file, targetExt);
}

export function buildDeclgenOutputPath(
    inputFile: string,
    moduleInfo: ModuleInfo,
    cacheDir?: string
): { declEtsOutputPath: string; glueCodeOutputPath: string } {
    let filePathFromModuleRoot: string;
    if (cacheDir && inputFile.endsWith(ETSCACHE_SUFFIX)) {
        filePathFromModuleRoot = path.relative(cacheDir, inputFile);
        const declEtsOutputPath: string = changeDeclgenFileExtension(
            path.resolve(moduleInfo.declgenV1OutPath!, filePathFromModuleRoot),
            DECL_ETS_SUFFIX
        );
        const glueCodeOutputPath: string = changeDeclgenFileExtension(
            path.resolve(moduleInfo.declgenBridgeCodePath!, filePathFromModuleRoot),
            TS_SUFFIX
        );
        ensurePathExists(declEtsOutputPath);
        ensurePathExists(glueCodeOutputPath);
        return { declEtsOutputPath, glueCodeOutputPath };
    }
    filePathFromModuleRoot = path.relative(moduleInfo.moduleRootPath, inputFile);
    const declEtsOutputPath: string = changeDeclgenFileExtension(
        path.resolve(moduleInfo.declgenV1OutPath!, moduleInfo.packageName, filePathFromModuleRoot),
        DECL_ETS_SUFFIX
    );
    const glueCodeOutputPath: string = changeDeclgenFileExtension(
        path.resolve(moduleInfo.declgenBridgeCodePath!, moduleInfo.packageName, filePathFromModuleRoot),
        TS_SUFFIX
    );
    ensurePathExists(declEtsOutputPath);
    ensurePathExists(glueCodeOutputPath);
    return { declEtsOutputPath, glueCodeOutputPath };
}

export function ensurePathExists(filePath: string): void {
    const dirPath: string = path.dirname(filePath);
    ensureDirExists(dirPath);
}

export function ensureDirExists(dirPath: string): void {
    try {
        if (!fs.existsSync(dirPath)) {
            fs.mkdirSync(dirPath, { recursive: true });
        }
    } catch (error) {
        if (error instanceof Error) {
            console.error(`Error: ${error.message}`);
        }
    }
}

export function toUnixPath(path: string): string {
    return path.replace(/\\/g, '/');
}

export function readFirstLineSync(filePath: string): string | null {

    const fd = fs.openSync(filePath, 'r');
    const buffer = Buffer.alloc(256);
    const bytesRead = fs.readSync(fd, buffer, 0, buffer.length, 0);
    fs.closeSync(fd);

    const content = buffer.toString('utf-8', 0, bytesRead);
    const firstLine = content.split(/\r?\n/, 1)[0].trim();

    return firstLine;
}

export function safeRealpath(path: string): string {
    try {
        return fs.realpathSync(path);
    } catch (error) {
        if (error instanceof Error) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL,
                    `Error resolving path "${path}".`,
                    error.message
                )
            );
        }
        throw error
    }
}

export function getInteropFilePathByApi(apiName: string, interopSDKPath: Set<string>): string {
    for (const sdkPath of interopSDKPath) {
        const modulePath = path.resolve(sdkPath, apiName + DECL_ETS_SUFFIX);
        if (fs.existsSync(modulePath)) {
            return modulePath;
        }
    }
    return '';
}

/**
 * Issue:26513
 * todo read config from external instead of prodcue
 */
export function getOhmurlByApi(api: string): string {
    const REG_SYSTEM_MODULE: RegExp = new RegExp(`@(${sdkConfigPrefix})\\.(\\S+)`);

    if (REG_SYSTEM_MODULE.test(api.trim())) {
        return api.replace(REG_SYSTEM_MODULE, (_, moduleType, systemKey) => {
            const systemModule: string = `${moduleType}.${systemKey}`;
            if (NATIVE_MODULE.has(systemModule)) {
                return `@native:${systemModule}`;
            } else if (moduleType === ARKTS_MODULE_NAME) {
                // @arkts.xxx -> @ohos:arkts.xxx
                return `@ohos:${systemModule}`;
            } else {
                return `@ohos:${systemKey}`;
            };
        });
    }
    return '';
}

export function isSubPathOf(targetPath: string, parentDir: string): boolean {
    const resolvedParent: string = path.posix.resolve(parentDir);
    const resolvedTarget: string = path.posix.resolve(targetPath);
    return resolvedTarget === resolvedParent || resolvedTarget.startsWith(resolvedParent + '/');
}

/**
 * Get the full extension of a file, supporting composite extensions like '.d.ts', '.test.ts', '.d.ets', etc.
 * @param filePath - File path or file name.
 * @param knownCompositeExts - Optional list of known composite extensions to match against.
 * @returns The full extension (e.g., '.d.ts'). Returns an empty string if no extension is found.
 */
export function getFileExtension(
    filePath: string,
    knownCompositeExts: string[] = ['.d.ts', '.test.ts', '.d.ets']
): string {
    const baseName = path.basename(filePath);

    // Match known composite extensions first
    for (const ext of knownCompositeExts) {
        if (baseName.endsWith(ext)) {
            return ext;
        }
    }

    // Fallback to default behavior: return the last segment after the final dot
    return path.extname(baseName);
}

export function hasEntry(moduleInfo: ModuleInfo): boolean {
    switch (moduleInfo.moduleType) {
        case OHOS_MODULE_TYPE.SHARED:
        case OHOS_MODULE_TYPE.HAR:
            return true;
        default:
            return false;
    }
}

export function createFileIfNotExists(filePath: string, content: string): boolean {
    try {
        const normalizedPath = path.normalize(filePath);
        if (fs.existsSync(normalizedPath)) {
            return false;
        }

        ensurePathExists(filePath);

        fs.writeFileSync(normalizedPath, content, { encoding: 'utf-8' });
        return true;
    } catch (error) {
        return false;
    }
}

export function isMixCompileProject(buildConfig: BuildConfig): boolean {
    for (const moduleInfo of buildConfig.dependencyModuleList) {
        if (
            moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1 ||
            moduleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID
        ) {
            return true;
        }
    }
    return false;
}

export function checkDependencyModuleInfoCorrectness(module: DependencyModuleConfig): boolean {
    return (module.packageName && module.modulePath && module.sourceRoots && module.entryFile) !== '';
}

export function computeHash(str: string): string {
    const hash = crypto.createHash('sha256');
    return hash.update(str).digest('hex');
}

export function getFileHash(filePath: string): string {
    return computeHash(fs.readFileSync(filePath, 'utf-8'));
}

export function updateFileHash(file: string, hashCache: Record<string, string>): boolean {
    const fileHash: string = getFileHash(file);
    const currHash: string = hashCache[file];
    if (fileHash === currHash) {
        return false;
    }

    Logger.getInstance().printDebug(`file ${file} hash changed: was ${currHash} became ${fileHash}`)

    hashCache[file] = fileHash;
    return true;

}

export function shouldBeUpdated(source: string, target: string): boolean {
    if (fs.existsSync(target)) {
        const sourceModified: number = fs.statSync(source).mtimeMs;
        const targetModified: number = fs.statSync(target).mtimeMs;
        if (sourceModified < targetModified) {
            return false;
        }
    }
    return true;
}

export function traverseDirAndFindFilesWithRegExp(dir: string, regexp: RegExp): string[] {
    if (!fs.existsSync(dir)) return [];
    let result: string[] = [];
    const entries = fs.readdirSync(dir, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            result = result.concat(traverseDirAndFindFilesWithRegExp(fullPath, regexp));
        } else if (regexp.test(entry.name)) {
            result.push(fullPath);
        }
    }
    return result;
}

function substituteEnvVarInString(str: string): string {
    if (str === '') {
        return str
    }

    const indexA = str.indexOf('${')
    const indexB = str.indexOf('}')

    if (indexA === -1 || indexB === -1) {
        return str
    }
    const envName: string = str.substring(indexA + 2, indexB)
    const envValue: string = process.env[envName] || ''

    if (envValue === '') {
        throw new Error(envName + ' environment variable is not set');
    }

    return str.replace(str.substring(indexA, indexB + 1), envValue)
}

export function substituteEnvVarsInJSON(json: any): any {
    Object.entries(json).forEach(([key, value]) => {
        if (typeof value === 'object') {
            json[key] = substituteEnvVarsInJSON(value);
        }
        if (typeof value === 'string') {
            json[key] = substituteEnvVarInString(value);
        }
    });

    return json;
}

export function validatePathLength(filePath: string, description: string): void {
    if (!isWindows()) {
        return;
    }
    if (filePath.length > MAX_PATH_LENGTH) {
        throw new DriverError(
            LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_PATH_TOO_LONG,
                `${description} exceeds maximum length.`,
                `Path length: ${filePath.length}, maximum: ${MAX_PATH_LENGTH}`,
                filePath
            )
        );
    }
}

export function sortAndDeduplicateStringArr(arr: string[]): string[] {
    if (arr.length === 0) {
        return arr;
    }
    arr.sort((a, b) => a.localeCompare(b));
    const tmpArr: string[] = [arr[0]];
    for (let i = 1; i < arr.length; i++) {
        if (arr[i] !== arr[i - 1]) {
            tmpArr.push(arr[i]);
        }
    }
    return tmpArr;
}

export function resolvePath(basePath: string, relativePath: string): string {
    if (relativePath.startsWith('/')) {
        return relativePath;
    }
    const baseDir = path.dirname(basePath);
    return path.resolve(baseDir, relativePath);
}

export function getAbsPathBaseConfigPath(configPath: string, relativePath: string): string {
    const absPath: string = path.join(path.dirname(configPath), relativePath);
    return toUnixPath(absPath);
}

export function containWildcards(item: string): boolean {
    return /[\*\?]/.test(item);
}

export function getProfilePath(srcPath: string): string {
    return `${srcPath.replace(/\$profile\:/, '')}.json`;
}

export function formatTimestamp(timestamp: number): string {
  const pad = (num: number, length: number = 2): string => {
    return num.toString().padStart(length, '0');
  };

  const date = new Date(timestamp);

  const year = date.getFullYear();
  const month = pad(date.getMonth() + 1);
  const day = pad(date.getDate());
  const hours = pad(date.getHours());
  const minutes = pad(date.getMinutes());
  const seconds = pad(date.getSeconds());
  const milliseconds = pad(date.getMilliseconds(), 3);
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}`;
}

export function getPid(): number {
  return process.pid;
}

let batchId: number = 0;

export function nextBatch(): void {
  batchId++;
}

export function getBatchId(): number {
  return batchId;
}
