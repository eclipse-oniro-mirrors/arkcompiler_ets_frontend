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

const GLUEGEN_SUBSYSTEM_CODE = '114' as const;

export type LogDataMoreInfo = Readonly<Record<string, unknown>>;

export interface LogDataInit {
  readonly code: string;
  readonly description: string;
  readonly cause?: string;
  readonly position?: string;
  readonly solutions?: readonly string[];
  readonly moreInfo?: LogDataMoreInfo;
}

export class LogData {
  public readonly code: string;
  public readonly description: string;
  public readonly cause: string;
  public readonly position: string;
  public readonly solutions: readonly string[];
  public readonly moreInfo?: LogDataMoreInfo;

  public constructor(init: LogDataInit) {
    if (!/^\d{8}$/.test(init.code)) {
      throw new Error(`Invalid hvigor error code: ${init.code}`);
    }
    this.code = init.code;
    this.description = init.description;
    this.cause = init.cause ?? '';
    this.position = init.position ?? '';
    this.solutions = [...(init.solutions ?? [])];
    if (init.moreInfo !== undefined) {
      this.moreInfo = { ...init.moreInfo };
    }
  }

  public toString(): string {
    return (
      `ERROR Code: ${this.code} ${this.description}\n` +
      this.errorContextText() +
      this.solutionsText() +
      this.moreInfoText()
    );
  }

  private errorContextText(): string {
    if (this.cause === '' && this.position === '') {
      return '';
    }
    let result = `Error Message: ${this.cause}\n`;
    if (this.position !== '') {
      result += `Position: ${this.position}\n`;
    }
    return `${result}\n\n`;
  }

  private solutionsText(): string {
    if (this.solutions.length === 0 || this.solutions[0] === '') {
      return '';
    }
    return `* Try the following: \n` + `${this.solutions.map((solution) => `  > ${solution}`).join('\n')}\n`;
  }

  private moreInfoText(): string {
    if (this.moreInfo === undefined) {
      return '';
    }
    let result = '\nMore Info:\n';
    for (const [key, value] of Object.entries(this.moreInfo)) {
      result += `  - ${key.toUpperCase()}: ${String(value)}\n`;
    }
    return result;
  }
}

export interface ILogger {
  printInfo(message: string): void;
  printWarn(message: string): void;
  printDebug(message: string): void;
  printError(data: LogData): void;
  printErrorAndExit(data: LogData): void;
}

export type LoggerGetter = (subsystemCode: typeof GLUEGEN_SUBSYSTEM_CODE) => ILogger;

/** A fallback for CLI and tests that do not provide an hvigor logger. */
export class ConsoleLogger implements ILogger {
  public printInfo(message: string): void {
    console.info('[INFO]', message);
  }

  public printWarn(message: string): void {
    console.warn('[WARN]', message);
  }

  public printDebug(message: string): void {
    console.debug('[DEBUG]', message);
  }

  public printError(data: LogData): void {
    console.error('[ERROR]', data.toString());
  }

  public printErrorAndExit(data: LogData): void {
    this.printError(data);
    process.exit(1);
  }
}

/** Creates an independent logger for one gluegen run. */
export function createRunLogger(loggerGetter?: LoggerGetter): ILogger {
  if (loggerGetter === undefined) {
    return new ConsoleLogger();
  }
  return loggerGetter(GLUEGEN_SUBSYSTEM_CODE);
}
