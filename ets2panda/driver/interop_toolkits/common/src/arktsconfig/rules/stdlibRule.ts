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

import * as path from 'node:path';

import type { ArkTSConfigRule } from './arktsconfigRule';
import { createRuleOutput, type RuleOutput } from './ruleOutput';

const ETS_STDLIB_PACKAGES = [
  'std/core',
  'std/math',
  'std/math/consts',
  'std/containers',
  'std/interop/js',
  'std/time',
  'std/debug',
  'std/debug/concurrency',
  'std/dfx',
  'std/testing',
  'std/concurrency',
  'std/annotations',
  'std/interop',
  'escompat',
  'arkruntime',
] as const;

export class StdlibRule implements ArkTSConfigRule {
  public constructor(private readonly pandaSdkPath: string) {}

  public async generate(): Promise<RuleOutput> {
    const output = createRuleOutput();
    const stdlibPath = path.resolve(this.pandaSdkPath, 'lib', 'etsstdlib.abc');
    for (const packageName of ETS_STDLIB_PACKAGES) {
      output.dependencies.set(packageName, {
        language: 'ets',
        path: stdlibPath,
        ohmUrl: packageName,
      });
    }
    return output;
  }
}
