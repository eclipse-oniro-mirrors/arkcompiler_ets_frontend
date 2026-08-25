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
import { collectFiles, pathExists } from './fileTree';
import { createRuleOutput, type RuleOutput } from './ruleOutput';

const ETS_DECLARATION_SUFFIX = '.d.ets';

export class ExternalSdkRule implements ArkTSConfigRule {
  public constructor(private readonly externalApiPaths: readonly string[]) {}

  public async generate(): Promise<RuleOutput> {
    const output = createRuleOutput();
    for (const sdkPath of this.externalApiPaths) {
      if (!(await pathExists(sdkPath))) {
        continue;
      }
      for (const filePath of await collectFiles(sdkPath, (candidate) => candidate.endsWith(ETS_DECLARATION_SUFFIX))) {
        const relativePath = path.relative(sdkPath, filePath);
        const parts = relativePath.split(path.sep);
        const fileName = parts.at(-1) ?? '';
        const basename = fileName.slice(0, -ETS_DECLARATION_SUFFIX.length);
        const runtimeApiIndex = parts.findIndex(
          (part, index) => part === 'arkui' && parts[index + 1] === 'runtime-api',
        );
        const key = runtimeApiIndex === -1 ? [...parts.slice(0, -1), basename].join('.') : basename;
        output.paths.set(key, [filePath.slice(0, -ETS_DECLARATION_SUFFIX.length)]);
      }
    }
    return output;
  }
}
