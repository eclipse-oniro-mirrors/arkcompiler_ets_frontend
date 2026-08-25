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

import type { ArkTSConfig } from './arktsconfig';
import type { ArkTSConfigContext } from './context';
import type { ArkTSConfigRule } from './rules/arktsconfigRule';
import { ExternalSdkRule } from './rules/externalSdkRule';
import { InteropSdkRule } from './rules/interopSdkRule';
import { ModuleRule } from './rules/moduleRule';
import { createRuleOutput, mergeRuleOutput } from './rules/ruleOutput';
import { StdlibRule } from './rules/stdlibRule';

export type { ArkTSConfigRule } from './rules/arktsconfigRule';
export { ExternalSdkRule } from './rules/externalSdkRule';
export { InteropSdkRule } from './rules/interopSdkRule';
export { ModuleRule, type ModuleRuleInput } from './rules/moduleRule';
export { StdlibRule } from './rules/stdlibRule';

export interface ArkTSConfigBuilderInput {
  readonly packageName: string;
  readonly baseUrl: string;
  readonly rootDir: string;
  readonly cachePath: string;
}

export class ArkTSConfigBuilder {
  private readonly rules: ArkTSConfigRule[] = [];

  public constructor(private readonly input: ArkTSConfigBuilderInput) {}

  public apply(rule: ArkTSConfigRule): this {
    this.rules.push(rule);
    return this;
  }

  public async build(): Promise<ArkTSConfig> {
    const output = createRuleOutput();
    for (const rule of this.rules) {
      mergeRuleOutput(output, await rule.generate());
    }
    return {
      compilerOptions: {
        package: this.input.packageName,
        baseUrl: this.input.baseUrl,
        rootDir: this.input.rootDir,
        paths: Object.fromEntries(output.paths),
        dependencies: Object.fromEntries(output.dependencies),
        cacheDir: this.input.cachePath,
        declgenV2OutPath: this.input.cachePath,
      },
    };
  }
}

export function createDefaultArkTSConfigBuilder(context: ArkTSConfigContext): ArkTSConfigBuilder {
  return new ArkTSConfigBuilder({
    packageName: context.mainModule.packageName,
    baseUrl: context.mainModule.modulePath,
    rootDir: context.projectRootPath,
    cachePath: context.cachePath,
  })
    .apply(
      new ModuleRule({
        modules: context.modules,
        modulesByPackage: context.byPackage,
        interopContexts: context.interopContexts,
        mainModule: context.mainModule,
        bundleName: context.bundleName,
        ...(context.moduleType === undefined ? {} : { moduleType: context.moduleType }),
      }),
    )
    .apply(new ExternalSdkRule(context.externalApiPaths))
    .apply(new InteropSdkRule(context.dynamicInteropSdkPaths))
    .apply(new StdlibRule(context.pandaSdkPath));
}
