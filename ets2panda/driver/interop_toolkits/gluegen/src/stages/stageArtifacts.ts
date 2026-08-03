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

import { createArtifactFactory } from '../pipeline/artifact';
import type { ArkTSConfigPath } from './arktsconfig';
import type { ConfigurationResult } from './configuration';
import type { InteropFileListPath } from './prepare';

/**
 * The single registry that binds stable artifact names to their value types.
 * Concrete hook payloads can evolve without changing the Pipeline framework.
 */
export interface GlueGenArtifactTypes {
  readonly configuration: ConfigurationResult;
  readonly interopFileList: InteropFileListPath;
  readonly arktsconfig: ArkTSConfigPath;
}

const createGlueGenArtifact = createArtifactFactory<GlueGenArtifactTypes>();

export const CONFIGURATION_ARTIFACT = createGlueGenArtifact('configuration');
export const INTEROP_FILE_LIST_ARTIFACT = createGlueGenArtifact('interopFileList');
export const ARKTS_CONFIG_ARTIFACT = createGlueGenArtifact('arktsconfig');
