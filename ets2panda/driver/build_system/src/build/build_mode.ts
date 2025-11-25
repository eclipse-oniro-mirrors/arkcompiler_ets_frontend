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
import { RecordEvent } from '../util/statsRecorder';

export class BuildMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }

    public async run(): Promise<void> {
        let buildMode = this.es2pandaMode
        if (buildMode === ES2PANDA_MODE.RUN_PARALLEL) {
            this.logger.printInfo('Run parallel')
            // RUN_PARALLEL: Executes tasks using multiple processes
            await super.runParallel();
        } else if (buildMode === ES2PANDA_MODE.RUN_CONCURRENT) {
            this.logger.printInfo('Run concurrent')
            // RUN_CONCURRENT: Executes tasks using multiple threads with ast-cache
            await super.runConcurrent();
        } else if (buildMode === ES2PANDA_MODE.RUN_SIMULTANEOUS) {
            this.logger.printInfo('Run simultaneous')
            // RUN_SIMULTANEOUS: Build with specific es2panda mode 'simultaneous'
            await super.runSimultaneous();
        } else if (buildMode === ES2PANDA_MODE.RUN) {
            this.logger.printInfo('Run ordinary')
            // RUN: Executes tasks sequentially in a single process and single thread
            await super.run();
        } else {
            this.logger.printInfo('Run parallel (default)')
            // Default fallback: same as RUN_PARALLEL
            await super.runParallel();
        }

        this.statsRecorder.record(RecordEvent.END);
        this.statsRecorder.writeSumSingle();
    }
}
