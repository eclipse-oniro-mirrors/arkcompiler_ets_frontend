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
const INTEROP_API_PREFIXES = 'ohos|system|kit|arkts';
const NATIVE_MODULES = new Set([
  'system.app',
  'ohos.app',
  'system.router',
  'system.curves',
  'ohos.curves',
  'system.matrix4',
  'ohos.matrix4',
]);

export class InteropSdkRule implements ArkTSConfigRule {
  public constructor(private readonly dynamicInteropSdkPaths: readonly string[]) {}

  public async generate(): Promise<RuleOutput> {
    const output = createRuleOutput();
    for (const sdkPath of await this.interopSdkPaths(this.dynamicInteropSdkPaths)) {
      if (path.basename(sdkPath) === 'kits') {
        continue;
      }
      const prefix = path.basename(sdkPath) === 'component' ? 'component/' : 'dynamic/';
      for (const filePath of await collectFiles(sdkPath, (candidate) => candidate.endsWith(ETS_DECLARATION_SUFFIX))) {
        const relativePath = path.relative(sdkPath, filePath);
        const parts = relativePath.split(path.sep);
        const fileName = parts.at(-1) ?? '';
        const basename = fileName.slice(0, -ETS_DECLARATION_SUFFIX.length);
        const runtimeApiIndex = parts.findIndex(
          (part, index) => part === 'arkui' && parts[index + 1] === 'runtime-api',
        );
        const separator = this.isInteropApiFile(fileName) ? '.' : '/';
        const relativeDirectory = runtimeApiIndex === -1 ? parts.slice(0, -1).join(separator) : '';
        const name = relativeDirectory === '' ? basename : `${relativeDirectory}${separator}${basename}`;
        output.dependencies.set(`${prefix}${name}`, {
          language: 'js',
          path: filePath,
          ohmUrl: this.getOhmUrlByApi(basename),
          alias: [name, `dynamic${name}`],
        });
      }
    }
    return output;
  }

  private async interopSdkPaths(configuredPaths: readonly string[]): Promise<readonly string[]> {
    const sdkPaths: string[] = [];
    for (const configuredPath of configuredPaths) {
      const basename = path.basename(configuredPath);
      if (['arkts', 'api', 'kits', 'component'].includes(basename)) {
        if (await pathExists(configuredPath)) {
          sdkPaths.push(configuredPath);
        }
        continue;
      }
      for (const directory of ['arkts', 'api', 'kits', 'component']) {
        const sdkPath = path.resolve(configuredPath, directory);
        if (await pathExists(sdkPath)) {
          sdkPaths.push(sdkPath);
        }
      }
    }
    return sdkPaths;
  }

  private isInteropApiFile(fileName: string): boolean {
    return new RegExp(`^@(${INTEROP_API_PREFIXES})\\..+\\.d\\.ets$`, 'i').test(fileName);
  }

  private getOhmUrlByApi(api: string): string {
    const pattern = new RegExp(`@(${INTEROP_API_PREFIXES})\\.(\\S+)`);
    if (!pattern.test(api.trim())) {
      return '';
    }
    return api.replace(pattern, (_match, moduleType: string, systemKey: string) => {
      const systemModule = `${moduleType}.${systemKey}`;
      if (NATIVE_MODULES.has(systemModule)) {
        return `@native:${systemModule}`;
      }
      return moduleType === 'arkts' ? `@ohos:${systemModule}` : `@ohos:${systemKey}`;
    });
  }
}
