/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { BaseMode } from './base_mode';
import {
    BuildConfig,
    ES2PANDA_MODE
} from '../types';

export class BuildMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }

    public async generateDeclaration(): Promise<void> {
        await super.generateDeclarationParallell();
    }

    public async run(): Promise<void> {
        if (this.es2pandaMode === ES2PANDA_MODE.RUN_PARALLEL) {
            // RUN_PARALLEL: Executes tasks using multiple processes
            await super.runParallel();
        } else if (this.es2pandaMode === ES2PANDA_MODE.RUN_CONCURRENT) {
            // RUN_CONCURRENT: Executes tasks using multiple threads with astcache
            await super.runConcurrent();
        } else if (this.es2pandaMode === ES2PANDA_MODE.RUN) {
            // RUN: Executes tasks sequentially in a single process and single thread
            await super.run();
        } else {
            // Default fallback: Uses parallel execution (same as RUN_PARALLEL)
            await super.run();
        }
    }
}
