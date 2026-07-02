/**
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

import { ArkTSConfigGenerator } from '../build/generate_arktsconfig';
import { ENABLE_CLUSTERS } from '../pre_define';
import { BuildConfig, ModuleInfo } from '../types';
import { DepAnalyzer, DependencyFileMap, DepGraphContext } from './dep_analyzer';

/**
 * Dependency Analyzer when full build
 */
export class FullDepAnalyzer extends DepAnalyzer {
    constructor(
        buildConfig: BuildConfig,
        generator: ArkTSConfigGenerator,
        clusteredBuild: boolean = ENABLE_CLUSTERS
    ) {
        super(buildConfig, generator, clusteredBuild);
    }

    protected createDepGraphContext(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        depMap: DependencyFileMap
    ): DepGraphContext {
        return {
            entryFiles,
            fileToModule,
            dependencyMap: depMap
        };
    }
}