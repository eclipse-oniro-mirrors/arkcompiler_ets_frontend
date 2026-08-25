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

import { createHvigorLogger, type ILogger, type LogData, type LoggerGetter } from '@interop-toolkits/common';

const SUBSYSTEM_CODE = '114';

class GlobalLogger implements ILogger {
  private delegate: ILogger = createHvigorLogger(SUBSYSTEM_CODE);

  initialize(loggerGetter?: LoggerGetter): void {
    this.delegate = createHvigorLogger(SUBSYSTEM_CODE, loggerGetter);
  }

  printInfo(message: string): void {
    this.delegate.printInfo(message);
  }

  printWarn(message: string): void {
    this.delegate.printWarn(message);
  }

  printDebug(message: string): void {
    this.delegate.printDebug(message);
  }

  printError(data: LogData): void {
    this.delegate.printError(data);
  }

  printErrorAndExit(data: LogData): void {
    this.delegate.printErrorAndExit(data);
  }
}

const globalLogger = new GlobalLogger();

export const logger: ILogger = globalLogger;

export function initializeLogger(loggerGetter?: LoggerGetter): void {
  globalLogger.initialize(loggerGetter);
}
