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

import { promises as fs } from 'node:fs';
import * as path from 'node:path';

import type { BuildConfig } from '../contracts';
import { GlueGenError, GlueGenErrorCode, GlueGenErrorList, GlueGenInternalError, errorMessage } from '../errors';
import { createRunLogger, LogData } from '../logger';
import { createArkTSConfigStage } from '../stages/arktsconfig';
import { createConfigurationStage } from '../stages/configuration';
import { createGenerationStage } from '../stages/generation';
import { createPrepareStage } from '../stages/prepare';
import type { GlueGenContext } from './context';
import { Pipeline } from './pipeline';

/** Per-run runtime settings; filesystem and process services are intentionally not injectable. */
export interface GlueGenRunnerOptions {
  readonly nativeExecutablePath?: string;
}

/** Internal owner of one glue generation lifecycle. */
export class GlueGenRunner {
  private readonly context: GlueGenContext;

  public constructor(buildConfig: BuildConfig, options: GlueGenRunnerOptions = {}) {
    this.context = {
      buildConfig,
      logger: createRunLogger(buildConfig.getHvigorConsoleLogger),
      runtime: {
        nativeExecutablePath: options.nativeExecutablePath ?? defaultNativeExecutablePath(),
      },
    };
  }

  public async run(): Promise<void> {
    try {
      await this.writeBuildConfig();
      const pipeline = Pipeline.start<GlueGenContext>()
        .stage(createConfigurationStage())
        .stage(createPrepareStage())
        .stage(createArkTSConfigStage())
        .stage(createGenerationStage());
      await pipeline.run(this.context);
    } catch (error) {
      if (error instanceof GlueGenErrorList) {
        error.errors.forEach((item) => this.context.logger.printError(item.logData));
        throw error;
      }
      if (error instanceof GlueGenError) {
        this.context.logger.printError(error.logData);
        throw error;
      }
      const data = new LogData({
        code: GlueGenErrorCode.INTERNAL_FAILURE,
        description: 'Gluegen stopped because of an unexpected wrapper failure.',
        cause: errorMessage(error, 'unknown gluegen failure'),
      });
      this.context.logger.printError(data);
      throw new GlueGenInternalError(data);
    }
  }

  private async writeBuildConfig(): Promise<void> {
    const { buildConfig } = this.context;
    const cachePath = path.isAbsolute(buildConfig.cachePath)
      ? buildConfig.cachePath
      : path.resolve(buildConfig.projectRootPath, buildConfig.cachePath);
    await fs.mkdir(cachePath, { recursive: true });
    await fs.writeFile(path.join(cachePath, 'projectionConfig.json'), JSON.stringify(buildConfig, null, 2), 'utf8');
  }
}

export function defaultNativeExecutablePath(): string {
  const executableName = process.platform === 'win32' ? 'gluegen.exe' : 'gluegen';
  return path.resolve(__dirname, '..', '..', 'bin', executableName);
}
