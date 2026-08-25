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
import * as path from 'node:path';
import { generateArktsconfig } from '../../utils';
import type { Context } from '../context';
import { ARKTS_CONFIG_ARTIFACT } from './stageArtifacts';

export function createArktsconfigStage(): common.framework.pipeline.ProvidedStage<
  Context,
  readonly [],
  typeof ARKTS_CONFIG_ARTIFACT
> {
  return common.framework.pipeline.Stage.start<Context>('arktsconfig')
    .use('generate-arktsconfig', {
      inputs: [],
      run: async (scope): Promise<string> => {
        const { buildConfig, arktsconfigPath } = scope.context;
        await generateArktsconfig(buildConfig, path.dirname(arktsconfigPath), arktsconfigPath);
        return arktsconfigPath;
      },
    })
    .provides(ARKTS_CONFIG_ARTIFACT, {
      build: (_scope, outputs): string => outputs['generate-arktsconfig'],
    });
}
