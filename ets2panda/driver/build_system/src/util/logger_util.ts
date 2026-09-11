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

import { logErrorMessage, Logger } from '../logger';
import { LogLevel, WorkerLogMessage } from '../types';

export function handleLogMessage(logger: Logger, logMsg: WorkerLogMessage): void {
    switch (logMsg.data.level) {
        case LogLevel.INFO:
            logger.printInfo(logMsg.data.message);
            break;
        case LogLevel.WARN:
            logger.printWarn(logMsg.data.message);
            break;
        case LogLevel.DEBUG:
            logger.printDebug(logMsg.data.message);
            break;
        case LogLevel.ERROR:
            logErrorMessage(logger, logMsg.data.error);
            break;
        case LogLevel.ERROR_AND_EXIT:
            logErrorMessage(logger, logMsg.data.error, true);
            break;
        default:
            break;
    }
}