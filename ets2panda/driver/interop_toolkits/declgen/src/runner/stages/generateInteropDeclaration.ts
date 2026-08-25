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

import * as common from '@interop-toolkits/common';
import * as adapter from '../../adapter';
import type { Context } from '../context';
import { INTEROP_CLOSURES_ARTIFACT, INTEROP_DECLARATIONS_ARTIFACT } from './stageArtifacts';

export function createGenerateInteropDeclarationStage(): common.framework.pipeline.ProvidedStage<
  Context,
  readonly [typeof INTEROP_CLOSURES_ARTIFACT],
  typeof INTEROP_DECLARATIONS_ARTIFACT
> {
  return common.framework.pipeline.Stage.start<Context>('generate-interop-declaration')
    .requires(INTEROP_CLOSURES_ARTIFACT)
    .use('generate-interop-declaration', {
      inputs: [],
      run: async (scope): Promise<void> => {
        const closures = scope.get(INTEROP_CLOSURES_ARTIFACT);
        const { buildConfig, fileManager, arktsconfigPath, tsconfigPath } = scope.context;
        const staticDeclgenAdapter = new adapter.StaticDeclgenAdapter(buildConfig, fileManager, arktsconfigPath);
        await staticDeclgenAdapter.run([...closures.staticClosure]);
        const dynamicDeclgenAdapter = new adapter.DynamicDeclgenAdapter(buildConfig, fileManager, tsconfigPath);
        await dynamicDeclgenAdapter.run([...closures.dynamicClosure]);
      },
    })
    .provides(INTEROP_DECLARATIONS_ARTIFACT, {
      build: (): undefined => undefined,
    });
}
