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

import { type LogDataInit, LogData } from './hvigorLogger';

export type ErrorMessage = Omit<LogDataInit, 'code'>;

/**
 * An error caused by user input, e.g. invalid configuration or command-line
 * arguments. The user can fix it by correcting the input, so the error message
 * should be actionable and user-facing.
 */
export class UserError extends Error {
  public constructor(public readonly errorMessage: ErrorMessage) {
    super(errorMessage.description);
    this.name = new.target.name;
  }

  public logData(code: string): LogData {
    return new LogData({ ...this.errorMessage, code });
  }
}

/**
 * Aggregates multiple user errors detected in one validation pass so that all
 * of them are reported before the run aborts. The wrapped errors are rendered
 * individually; this container itself carries no user-facing details.
 */
export class AggregateUserError extends UserError {
  public constructor(public readonly errors: readonly UserError[]) {
    super({ description: `Multiple errors occurred: ${errors.length} error(s) found.` });
  }
}

/**
 * An error caused by the internal implementation or other unexpected causes,
 * e.g. a broken invariant or an environment failure. It is not the user's
 * fault and usually indicates a bug that should be reported to maintainers.
 */
export class InternalError extends Error {
  public constructor(message: string) {
    super(message);
    this.name = new.target.name;
  }

  public logData(code: string): LogData {
    return new LogData({ description: this.message, code });
  }
}
