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

import * as path from 'node:path';
import type { BuildConfig } from '../buildConfig';
import type { Context } from './context';
import * as common from '@interop-toolkits/common';
import * as predefine from '../predefine';
import * as utils from '../utils';
import * as errors from '../errors';
import { logger } from '../logger';
import { createArktsconfigStage } from './stages/arktsconfig';
import { createComputeInteropClosureStage } from './stages/computeInteropClosure';
import { createGenerateInteropDeclarationStage } from './stages/generateInteropDeclaration';
import { createResolveDependencyGraphStage } from './stages/resolveDependencyGraph';
import { createResolveInteropEntriesStage } from './stages/resolveInteropEntries';

export class DeclgenRunner {
  private context: Context;

  constructor(buildConfig: BuildConfig) {
    const fileManager = new common.fileManager.FileManagerBuilder()
      .addDynamicSdkPaths(buildConfig.sdkPaths.dynamicSdkPaths)
      .addStaticSdkPaths(buildConfig.sdkPaths.staticSdkPaths)
      .addDynamicInteropSdkPaths(buildConfig.sdkPaths.dynamicInteropSdkPaths)
      .addStaticInteropSdkPaths(buildConfig.sdkPaths.staticInteropSdkPaths)
      .addModuleList(buildConfig.dependentModuleList)
      .build();
    const cacheDir = buildConfig.cachePath;
    const arktsconfigPath = path.join(cacheDir, predefine.ARKTSCONFIG_FILE_NAME);
    const tsconfigPath = utils.getDynamicTsConfigPath(buildConfig);
    this.context = { buildConfig, fileManager, tsconfigPath, arktsconfigPath };
  }

  public async run(): Promise<void> {
    try {
      const pipeline = common.framework.pipeline.Pipeline.start<Context>()
        .stage(createResolveInteropEntriesStage())
        .stage(createArktsconfigStage())
        .stage(createResolveDependencyGraphStage())
        .stage(createComputeInteropClosureStage())
        .stage(createGenerateInteropDeclarationStage());
      await pipeline.run(this.context);
    } catch (error) {
      if (error instanceof common.errors.InternalError) {
        logger.printError(error.logData(errors.ErrorCode.DECLGEN_INTERNAL_ERROR));
      } else if (error instanceof common.interopConfig.InteropConfigError) {
        logger.printError(error.logData(errors.ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG));
      } else if (error instanceof errors.SentinelNotConfiguredError) {
        logger.printError(error.logData(errors.ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG));
      }
      throw error;
    }
  }
}
