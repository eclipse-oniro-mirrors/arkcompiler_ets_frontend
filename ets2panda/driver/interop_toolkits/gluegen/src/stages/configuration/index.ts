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

import type { GlueGenContext } from '../../pipeline/context';
import { Stage } from '../../pipeline';
import { CONFIGURATION_ARTIFACT } from '../stageArtifacts';
import type { ConfigurationResult } from './configurationResult';
import { resolveInteropConfig } from './resolveInteropConfig';
import { resolveModules } from './resolveModules';
import { validate } from './validate';

export function createConfigurationStage() {
  return Stage.start<GlueGenContext>('configuration')
    .use('validate', {
      inputs: [],
      run: validate,
    })
    .use('resolve-modules', {
      inputs: [],
      run: resolveModules,
    })
    .use('resolve-interop-config', {
      inputs: ['resolve-modules'],
      run: resolveInteropConfig,
    })
    .provides(CONFIGURATION_ARTIFACT, {
      build: (_scope, outputs): ConfigurationResult => ({
        moduleTable: outputs['resolve-modules'],
        interopTargets: outputs['resolve-interop-config'],
      }),
    });
}

export { dependencyModulesOf, reachableModulesOf } from './moduleTable';
export type { ConfigurationResult } from './configurationResult';
export type { InteropTarget } from './interopTarget';
export type { MainModuleInfo, ModuleInfo, ModuleTable } from './moduleTable';
