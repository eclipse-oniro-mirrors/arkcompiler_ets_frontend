/*
 * Copyright (c) 2024 - 2025 Huawei Device Co., Ltd.
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

import { ArkFile } from 'arkanalyzer';
import { FileMatcher, isMatchedFile } from '../Matchers';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'matchFiles');

export function matchFiles(arkFiles: ArkFile[], matcher: FileMatcher, callback: Function): void {
    for (let arkFile of arkFiles) {
        if (isMatchedFile(arkFile, [matcher])) {
            try {
                callback(arkFile);
            } catch (error) {
                logger.error('Error in file callback: ', error);
            }
        }
    }
}