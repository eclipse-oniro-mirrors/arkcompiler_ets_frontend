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

import { BuildConfig } from '../buildConfig';
import * as common from '@interop-toolkits/common';

export abstract class DeclgenAdapter {
  constructor(
    protected readonly buildConfig: BuildConfig,
    protected readonly fileManager: common.fileManager.FileManager,
    protected readonly compilerConfigPath: string,
  ) {}
  abstract run(entryFiles: string[]): Promise<void>;
}
