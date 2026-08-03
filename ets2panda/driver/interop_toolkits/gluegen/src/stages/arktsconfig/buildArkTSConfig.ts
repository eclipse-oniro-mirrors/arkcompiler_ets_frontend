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

import type { BuildConfig } from '../../contracts';
import type { ModuleTable } from '../configuration';
import type { ArkTSConfig, ArkTSConfigPath } from './arktsconfig';
import type { ArkTSConfigRule, GenerationContext } from './rules/arktsconfigRule';
import { ExternalSdkRule } from './rules/externalSdkRule';
import { InteropSdkRule } from './rules/interopSdkRule';
import { ModuleRule } from './rules/moduleRule';
import { createRuleOutput, mergeRuleOutput } from './rules/ruleOutput';
import { StdlibRule } from './rules/stdlibRule';

const ARKTS_CONFIG_RULES: readonly ArkTSConfigRule[] = [
  new ModuleRule(),
  new ExternalSdkRule(),
  new InteropSdkRule(),
  new StdlibRule(),
];

/** Builds the main module's ArkTS Configuration from normalized project inputs. */
export async function buildArkTSConfig(buildConfig: BuildConfig, moduleTable: ModuleTable): Promise<ArkTSConfig> {
  const context: GenerationContext = {
    buildConfig,
    moduleTable,
  };
  const output = createRuleOutput();
  for (const rule of ARKTS_CONFIG_RULES) {
    mergeRuleOutput(output, await rule.generate(context));
  }

  const mainModule = moduleTable.mainModule;
  return {
    compilerOptions: {
      package: mainModule.packageName,
      baseUrl: mainModule.modulePath,
      rootDir: mainModule.projectRootPath,
      paths: Object.fromEntries(output.paths),
      dependencies: Object.fromEntries(output.dependencies),
      cacheDir: mainModule.cachePath,
      declgenV2OutPath: mainModule.cachePath,
    },
  };
}

export function createArkTSConfigPath(moduleTable: ModuleTable): ArkTSConfigPath {
  return path.join(
    moduleTable.mainModule.cachePath,
    moduleTable.mainModule.packageName,
    'arktsconfig.json',
  ) as ArkTSConfigPath;
}

export function serializeArkTSConfig(config: ArkTSConfig): string {
  return JSON.stringify(config, null, 2);
}
