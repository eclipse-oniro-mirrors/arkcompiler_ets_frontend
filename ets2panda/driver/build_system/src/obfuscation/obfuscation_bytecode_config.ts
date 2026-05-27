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

import { BuildConfig } from '../types';
import { MergedConfig } from './obfuscation_merged_config';
import { containWildcards } from '../util/utils';
import { MERGED_ABC_FILE } from '../pre_define';
import { Logger } from '../logger';

export class BytecodeObfuscationConfig {
    abcPath: string;
    obfAbcPath: string;
    defaultNameCachePath: string;
    obfuscationRules: {
        disableObfuscation: boolean;
        printNameCache: string;
        applyNameCache: string;
        applyNameCacheDecl: string;
        removeLog: boolean;
        printSeedsOption: {
            enable: boolean;
            filePath: string;
        };
        fileNameObfuscation: {
            enable: boolean;
            reservedFileNames: Set<string>;
            universalReservedFileNames: Set<string>;
        };
        keepOptions: {
            keepPath: {
                reservedPaths: Set<string>;
                universalReservedPaths: Set<string>;
            }
            keeps: Set<string>;
        };
    };

    constructor(buildConfig: BuildConfig, mergedConfig: MergedConfig) {
        const mergedObConfig: MergedConfig = mergedConfig;
        const obfuscationCacheDir: string = buildConfig?.obfuscationOptions?.obfuscationCacheDir || '';
        const obfDir: string = path.join(obfuscationCacheDir, 'obf');
        this.abcPath = path.resolve(buildConfig.loaderOutPath, MERGED_ABC_FILE);
        this.obfAbcPath = path.join(obfDir, MERGED_ABC_FILE);
        this.defaultNameCachePath = path.join(obfuscationCacheDir, 'nameCache.json');
        let reservedPaths = [...mergedObConfig.keepSourceOfPaths]
        const universalReservedPaths = [...mergedObConfig.keepUniversalPaths.map(regexp => regexp.toString())];
        const fileNameReservedInfo: ReservedNameInfo = this.separateUniversalReservedItem(mergedConfig.reservedFileNames);
        const systemApiCachePath: string = path.join(obfuscationCacheDir, 'systemApiCache.txt');
        const systemApiWhitelist: string[] = this.getSystemAPIWhitelist(systemApiCachePath);
        this.obfuscationRules = {
            disableObfuscation: mergedObConfig.options.disableObfuscation,
            printNameCache: mergedObConfig.options.printNameCache,
            applyNameCache: mergedObConfig.options.applyNameCache,
            applyNameCacheDecl: mergedObConfig.options.applyNameCacheDecl,
            removeLog: mergedObConfig.options.removeLog,
            fileNameObfuscation: {
                enable: mergedObConfig.options.enableFileNameObfuscation,
                reservedFileNames: new Set<string>(fileNameReservedInfo.specificReservedArray),
                universalReservedFileNames: new Set(
                    fileNameReservedInfo.universalReservedArray?.map(regexp => regexp.toString()) || []
                )
            },
            printSeedsOption: {
                enable: mergedObConfig.options.enablePrintSeeds,
                filePath: mergedObConfig.options.printSeedsFilePath,
            },
            keepOptions: {
                keepPath: {
                    reservedPaths: new Set<string>(reservedPaths),
                    universalReservedPaths: new Set<string>(universalReservedPaths),
                },
                keeps: new Set<string>([
                    ...systemApiWhitelist,
                    ...mergedObConfig.keepClassSpecLists,
                    ...mergedObConfig.keepMembers,
                    ...mergedObConfig.keepClassWithMembers
                ])
            }
        };
    }

    separateUniversalReservedItem(originalArray: string[] | undefined): ReservedNameInfo {
        const reservedInfo: ReservedNameInfo = {
            universalReservedArray: [],
            specificReservedArray: []
        };
        if (!originalArray) {
            return reservedInfo;
        }
        originalArray.forEach(reservedItem => {
            if (containWildcards(reservedItem)) {
                reservedInfo.universalReservedArray.push(reservedItem);
            } else {
                reservedInfo.specificReservedArray.push(reservedItem);
            }
        });
        return reservedInfo;
    }

    getSystemAPIWhitelist(whitelistPath: string): string[] {
        try {
            if (!whitelistPath || !fs.existsSync(whitelistPath)) {
                return [];
            }
            const data = fs.readFileSync(whitelistPath, 'utf8');
            return data.split('\n').map(line => line.trim()).filter(line => line !== '').map(line => line = '-keep ' + line);
        } catch (error) {
            Logger.getInstance().printWarn(`Error reading ArkUI whitelist file: ${whitelistPath}`);
            return [];
        }
    }
}

export interface ReservedNameInfo {
    universalReservedArray: string[];
    specificReservedArray: string[];
}