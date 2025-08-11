/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { BaseMode } from './base_mode';
import { BuildConfig, ModuleInfo } from '../types';
import {  LogDataFactory } from '../logger';
import { changeFileExtension } from '../util/utils';
import { ABC_SUFFIX } from '../pre_define';
import path from 'path';
import { ErrorCode, DriverError } from '../util/error';

export class BuildFrameworkMode extends BaseMode {

    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
        this.mergedAbcFile = buildConfig.loaderOutPath as string;
    }

    public async runSimultaneous(): Promise<void> {
        await super.runSimultaneous();
    }

    // NOTE: never called
    protected parseBuildConfig(): void {
        this.collectModuleInfos();
        this.generateArkTSConfigForModules();
        this.processEntryFiles();
    }

    protected processEntryFiles(): void {
        this.entryFiles.forEach((file: string) => {
            for (const [_, moduleInfo] of this.moduleInfos) {
                if (!file.startsWith(moduleInfo.moduleRootPath)) {
                    continue;
                }
                let filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, file);
                let filePathInCache: string = path.join(this.cacheDir, moduleInfo.packageName, filePathFromModuleRoot);
                let abcFilePath: string = path.resolve(changeFileExtension(filePathInCache, ABC_SUFFIX));
                this.abcFiles.add(abcFilePath);
                this.fileToModule.set(file, moduleInfo);
                return;
            }
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
                    'File does not belong to any module in moduleInfos.',
                    '',
                    file
                )
            );
        });
    }

    // NOTE: never called
    protected getMainModuleInfo(): ModuleInfo {
        let moduleInfo = super.getMainModuleInfo();
        moduleInfo.frameworkMode = this.frameworkMode;
        moduleInfo.useEmptyPackage = this.useEmptyPackage;
        return moduleInfo;
    }
}
