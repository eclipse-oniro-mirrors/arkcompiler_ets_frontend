/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { logger, initializeLogger } from './logger';
import type { BuildConfig } from './buildConfig';
import * as fs from 'fs';
import * as path from 'path';
import * as utils from './utils';
import { DeclgenRunner } from './runner/runner';

export async function runDeclgen(buildConfig: BuildConfig): Promise<void> {
  utils.initBuildConfig(buildConfig);
  initializeLogger(buildConfig.getHvigorConsoleLogger);
  logger.printInfo('Generating interop declaration files...');
  await writeProjectConfigIntoCache(buildConfig);
  await new DeclgenRunner(buildConfig).run();
}

async function writeProjectConfigIntoCache(buildConfig: BuildConfig): Promise<void> {
  const cacheDir = buildConfig.cachePath;
  if (!fs.existsSync(cacheDir)) {
    await fs.promises.mkdir(cacheDir, { recursive: true });
  }
  const configPath = path.join(cacheDir, 'buildConfig.json');
  await fs.promises.writeFile(configPath, JSON.stringify(buildConfig, null, 2), 'utf-8');
}
