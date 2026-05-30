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

interface BytecodeObf {
    enable: boolean;
    debugging: boolean;
    configPath: string;
}

export class ObOptions {
    bytecodeObf: BytecodeObf = {
        enable: false,
        debugging: false,
        configPath: '',
    };
    disableObfuscation: boolean = false;
    printConfiguration: string | undefined;
    removeLog: boolean = false;

    enableKeep: boolean = false;
    enableKeepClassWithMembers: boolean = false;
    enableKeepMembers: boolean = false;

    printNameCache: string = '';
    applyNameCache: string = '';
    applyNameCacheDecl: string = '';
    enablePrintSeeds: boolean = false;
    printSeedsFilePath: string = '';
    enableFileNameObfuscation: boolean = true;

    mergeObOptions(other: ObOptions): void {
        this.disableObfuscation = this.disableObfuscation || other.disableObfuscation;
        this.removeLog = this.removeLog || other.removeLog;
        this.enablePrintSeeds = this.enablePrintSeeds || other.enablePrintSeeds;
        this.enableKeep = this.enableKeep || other.enableKeep;
        this.enableKeepClassWithMembers = this.enableKeepClassWithMembers || other.enableKeepClassWithMembers;
        this.enableKeepMembers = this.enableKeepMembers || other.enableKeepMembers;
        if (other.printNameCache.length > 0) {
            this.printNameCache = other.printNameCache;
        }
        if (other.applyNameCache.length > 0) {
            this.applyNameCache = other.applyNameCache;
        }
        if (other.applyNameCacheDecl.length > 0) {
            this.applyNameCacheDecl = other.applyNameCacheDecl;
        }
        if (other.printSeedsFilePath.length > 0) {
            this.printSeedsFilePath = other.printSeedsFilePath;
        }
        if (other.printConfiguration && other.printConfiguration.length > 0) {
            this.printConfiguration = other.printConfiguration;
        }
    }
}