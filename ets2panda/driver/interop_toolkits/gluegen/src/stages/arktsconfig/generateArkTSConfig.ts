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

import { promises as fs } from 'node:fs';
import * as path from 'node:path';

import { GlueGenError, GlueGenErrorCode, GlueGenInternalError, errorMessage } from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import { CONFIGURATION_ARTIFACT } from '../stageArtifacts';
import { buildArkTSConfig, createArkTSConfigPath, serializeArkTSConfig } from './buildArkTSConfig';
import type { ArkTSConfigPath } from './arktsconfig';

type ArkTSConfigRequirements = readonly [typeof CONFIGURATION_ARTIFACT];

/** Creates the main module's arktsconfig.json and publishes its path. */
export async function generateArkTSConfig(
  scope: StageScope<GlueGenContext, ArkTSConfigRequirements>,
): Promise<ArkTSConfigPath> {
  const { context } = scope;
  const { moduleTable } = scope.get(CONFIGURATION_ARTIFACT);
  const outputPath = createArkTSConfigPath(moduleTable);
  try {
    const arkTSConfig = await buildArkTSConfig(context.buildConfig, moduleTable);
    await fs.mkdir(path.dirname(outputPath), { recursive: true });
    await fs.writeFile(outputPath, serializeArkTSConfig(arkTSConfig), 'utf8');
    return outputPath;
  } catch (error) {
    if (error instanceof GlueGenError) {
      throw error;
    }
    const data = new LogData({
      code: GlueGenErrorCode.GENERATE_ARKTS_CONFIG_FAIL,
      description: 'Gluegen could not generate the ArkTS configuration.',
      cause: errorMessage(error, 'unknown arktsconfig failure'),
      position: outputPath,
    });
    throw new GlueGenInternalError(data);
  }
}
