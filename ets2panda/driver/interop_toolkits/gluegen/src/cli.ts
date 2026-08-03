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

import { readFile } from 'node:fs/promises';
import * as path from 'node:path';

import { runGluegen } from './entry';
import type { BuildConfig } from './contracts';

async function main(arguments_: readonly string[]): Promise<void> {
  const buildConfigArgument = arguments_[0];
  if (buildConfigArgument === undefined) {
    throw new Error('usage: gluegen <build-config.json>');
  }
  const buildConfigPath = path.resolve(buildConfigArgument);
  const serialized = await readFile(buildConfigPath, 'utf8');
  const buildConfig = JSON.parse(serialized) as BuildConfig;
  await runGluegen(buildConfig);
}

if (require.main === module) {
  main(process.argv.slice(2)).catch(() => {
    process.exitCode = 1;
  });
}
