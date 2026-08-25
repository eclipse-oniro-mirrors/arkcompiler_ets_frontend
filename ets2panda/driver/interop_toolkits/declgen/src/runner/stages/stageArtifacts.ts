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
import type * as dependencyResolver from 'dependency-resolver';

export interface InteropEntryFiles {
  readonly staticEntryFiles: Set<string>;
  readonly dynamicEntryFiles: Set<string>;
}

export interface InteropClosures {
  readonly staticClosure: Set<string>;
  readonly dynamicClosure: Set<string>;
}

interface ArtifactTypes {
  readonly arktsconfig: string;
  readonly interopEntryFiles: InteropEntryFiles;
  readonly dependencyGraph: dependencyResolver.DependencyGraph;
  readonly interopClosures: InteropClosures;
  readonly interopDeclarations: void;
}

const createArtifact = common.framework.pipeline.createArtifactFactory<ArtifactTypes>();

export const ARKTS_CONFIG_ARTIFACT = createArtifact('arktsconfig');
export const INTEROP_ENTRY_FILES_ARTIFACT = createArtifact('interopEntryFiles');
export const DEPENDENCY_GRAPH_ARTIFACT = createArtifact('dependencyGraph');
export const INTEROP_CLOSURES_ARTIFACT = createArtifact('interopClosures');
export const INTEROP_DECLARATIONS_ARTIFACT = createArtifact('interopDeclarations');
