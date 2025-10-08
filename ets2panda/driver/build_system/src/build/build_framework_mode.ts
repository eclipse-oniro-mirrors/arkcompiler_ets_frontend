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
import { BuildConfig, CompileFileInfo, ModuleInfo } from '../types';
import { LogData, LogDataFactory } from '../logger';
import { changeFileExtension } from '../util/utils';
import { ABC_SUFFIX } from '../pre_define';
import path from 'path';
import { ErrorCode } from '../error_code';

export class BuildFrameworkMode extends BaseMode {
  frameworkMode: boolean;
  useEmptyPackage: boolean;

  constructor(buildConfig: BuildConfig) {
    super(buildConfig);
    this.mergedAbcFile = buildConfig.loaderOutPath as string;
    this.frameworkMode = buildConfig.frameworkMode ?? false;
    this.useEmptyPackage = buildConfig.useEmptyPackage ?? false;
  }

  public async run(): Promise<void> {
    super.run();
  }

  protected generateModuleInfos(): void {
    this.collectModuleInfos();
    this.generateArkTSConfigForModules();
    this.collectCompileFiles();
  }

  protected collectCompileFiles(): void {
    this.entryFiles.forEach((file: string) => {
      for (const [packageName, moduleInfo] of this.moduleInfos) {
        if (!file.startsWith(moduleInfo.moduleRootPath)) {
          continue;
        }
        let filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, file);
        let filePathInCache: string = path.join(this.cacheDir, moduleInfo.packageName, filePathFromModuleRoot);
        let abcFilePath: string = path.resolve(changeFileExtension(filePathInCache, ABC_SUFFIX));
        this.abcFiles.add(abcFilePath);
        let fileInfo: CompileFileInfo = {
          filePath: file,
          dependentFiles: [],
          abcFilePath: abcFilePath,
          arktsConfigFile: moduleInfo.arktsConfigFile,
          packageName: moduleInfo.packageName
        };
        moduleInfo.compileFileInfos.push(fileInfo);
        this.compileFiles.set(file, fileInfo);
        return;
      }
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
        'File does not belong to any module in moduleInfos.',
        '',
        file
      );
      this.logger.printError(logData);
    });
  }

  protected getMainModuleInfo(): ModuleInfo {
    let moduleInfo = super.getMainModuleInfo();
    moduleInfo.frameworkMode = this.frameworkMode;
    moduleInfo.useEmptyPackage = this.useEmptyPackage;
    return moduleInfo;
  }
}
