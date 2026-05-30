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

import { sortAndDeduplicateStringArr } from '../util/utils';
import { ObConfigResolver } from './obfuscation_config_resolver';
import { ObOptions } from './obfuscation_options';

export class MergedConfig {
    options: ObOptions = new ObOptions();

    reservedFileNames: string[] = [];
    keepClassSpecLists: string[] = [];
    keepClassWithMembers: string[] = [];
    keepMembers: string[] = [];
    keepUniversalPaths: string[] = [];
    keepSourceOfPaths: string[] = [];
    excludeUniversalPaths: string[] = [];
    excludePathSet: Set<string> = new Set();

    mergeKeepOptions(other: MergedConfig): void {
        this.reservedFileNames.push(...other.reservedFileNames);
        this.keepClassSpecLists.push(...other.keepClassSpecLists);
        this.keepClassWithMembers.push(...other.keepClassWithMembers);
        this.keepMembers.push(...other.keepMembers);
        this.keepUniversalPaths.push(...other.keepUniversalPaths);
        this.keepSourceOfPaths.push(...other.keepSourceOfPaths);
        this.excludeUniversalPaths.push(...other.excludeUniversalPaths);
        other.excludePathSet.forEach((excludePath) => {
            this.excludePathSet.add(excludePath);
        });
    }

    mergeAllRules(other: MergedConfig): void {
        this.options.mergeObOptions(other.options);
        this.mergeKeepOptions(other);
    }

    sortAndDeduplicate(): void {
        this.reservedFileNames = sortAndDeduplicateStringArr(this.reservedFileNames);
        this.keepClassSpecLists = sortAndDeduplicateStringArr(this.keepClassSpecLists);
        this.keepClassWithMembers = sortAndDeduplicateStringArr(this.keepClassWithMembers);
        this.keepMembers = sortAndDeduplicateStringArr(this.keepMembers);
        this.keepSourceOfPaths = sortAndDeduplicateStringArr(this.keepSourceOfPaths);
        this.keepUniversalPaths = sortAndDeduplicateStringArr(this.keepUniversalPaths);
    }

    printMergedConfig(): string {
        let resultStr: string = '';
        const keys = Object.keys(this.options) as Array<keyof typeof this.options>;
        resultStr += '# Obfuscation Configuration\n';
        resultStr += '# =================================\n\n';
        resultStr += '# Options:\n';
        for (const key of keys) {
            const value = this.options[key];
            if (typeof value === 'boolean') {
                resultStr += `# ${key}: ${value}\n`;
            } else if (typeof value === 'object') {
                resultStr += `# ${key}: ${JSON.stringify(value)}\n`;
            } else if (value !== '') {
                resultStr += `# ${key}: ${value}\n`;
            }
        }
        resultStr += '\n# Keep Options:\n';
        resultStr += `# reservedFileNames: ${JSON.stringify(this.reservedFileNames)}\n`;
        resultStr += `# keepClassSpecLists: ${JSON.stringify(this.keepClassSpecLists)}\n`;
        resultStr += `# keepClassWithMembers: ${JSON.stringify(this.keepClassWithMembers)}\n`;
        resultStr += `# keepMembers: ${JSON.stringify(this.keepMembers)}\n`;
        resultStr += `# keepUniversalPaths: ${JSON.stringify(this.keepUniversalPaths)}\n`;
        resultStr += `# keepSourceOfPaths: ${JSON.stringify(this.keepSourceOfPaths)}\n`;
        resultStr += `# excludeUniversalPaths: ${JSON.stringify(this.excludeUniversalPaths)}\n`;
        resultStr += `# excludePathSet: ${JSON.stringify([...this.excludePathSet])}\n`;
        return resultStr;
    }

    serializeMergedConfig(): string {
        const result: string[] = [];
        const keys = Object.keys(this.options) as Array<keyof typeof this.options>;
        for (const key of keys) {
            const optionValue = this.options[key];
            if (optionValue === true && ObConfigResolver.exportedSwitchMap.has(key as string)) {
                const exportValue = ObConfigResolver.exportedSwitchMap.get(key as string);
                if (exportValue) {
                    result.push(exportValue);
                }
            }
        }
        const addKeepOptions = (option: boolean, title: string, items: string[], newTitle: boolean = false) : void => {
            if (option && items.length > 0) {
                if (newTitle) {
                    result.push(title);
                }
                result.push(...items);
            }
        };
        const keepPaths = [...this.keepSourceOfPaths, ...this.keepUniversalPaths];
        addKeepOptions(this.options.enableKeep, ObConfigResolver.KEEP, keepPaths, true);
        addKeepOptions(this.options.enableKeep, ObConfigResolver.KEEP, this.keepClassSpecLists);
        addKeepOptions(this.options.enableKeepClassWithMembers, ObConfigResolver.KEEP_CLASS_WITH_MEMBERS_ARKGUARD, this.keepClassWithMembers);
        addKeepOptions(this.options.enableKeepMembers, ObConfigResolver.KEEP_CLASS_MEMBERS_ARKGUARD, this.keepMembers);
        return result.join('\n') + '\n';
    }
}