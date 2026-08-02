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

import { collectFiles, pathExists } from '../../../utils/fileTree';
import type { ArkTSConfigRule, GenerationContext } from './arktsconfigRule';
import { createRuleOutput, type RuleOutput } from './ruleOutput';

const ETS_DECLARATION_SUFFIX = '.d.ets';

/** Generates paths contributed by the static external SDK. */
export class ExternalSdkRule implements ArkTSConfigRule {
  async generate(context: GenerationContext): Promise<RuleOutput> {
    const output = createRuleOutput();
    for (const sdkPath of context.buildConfig.externalApiPaths) {
      if (!(await pathExists(sdkPath))) {
        continue;
      }
      for (const filePath of await collectFiles(sdkPath, (candidate) => candidate.endsWith(ETS_DECLARATION_SUFFIX))) {
        output.paths.set(this.keyFor(sdkPath, filePath), [filePath.slice(0, -ETS_DECLARATION_SUFFIX.length)]);
      }
    }
    return output;
  }

  private keyFor(sdkPath: string, filePath: string): string {
    const relativePath = path.relative(sdkPath, filePath);
    const parts = relativePath.split(path.sep);
    const fileName = parts.at(-1) ?? '';
    const basename = fileName.slice(0, -ETS_DECLARATION_SUFFIX.length);
    const runtimeApiIndex = parts.findIndex((part, index) => part === 'arkui' && parts[index + 1] === 'runtime-api');
    if (runtimeApiIndex !== -1) {
      return basename;
    }
    return [...parts.slice(0, -1), basename].join('.');
  }
}
