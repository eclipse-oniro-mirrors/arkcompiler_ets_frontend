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

import { LogData } from '../logger';
import { ErrorCode } from '../util/error';

/** Structured extra fields attached to a user-facing error. */
export type LogDataMoreInfo = Readonly<Record<string, unknown>>;

/**
 * The user-facing part of a `LogData`. Unlike the `LogData` class, all fields
 * except `description` are optional, matching how partial messages are thrown.
 */
export interface ErrorMessage {
  readonly description: string;
  readonly cause?: string;
  readonly position?: string;
  readonly solutions?: readonly string[];
  readonly moreInfo?: LogDataMoreInfo;
}

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

  public logData(code: ErrorCode): LogData {
    return new LogData(
      code,
      this.errorMessage.description,
      this.errorMessage.cause ?? '',
      this.errorMessage.position ?? '',
      this.errorMessage.solutions === undefined ? [] : [...this.errorMessage.solutions],
      this.errorMessage.moreInfo,
    );
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

  public logData(code: ErrorCode): LogData {
    return new LogData(code, this.message, '', '', []);
  }
}


/** Returns an Error message while preserving a caller-specific fallback for non-Error throws. */
export function errorMessage(error: unknown, fallback: string): string {
  return error instanceof Error ? error.message : fallback;
}

export class InteropConfigError extends UserError {
  public constructor(errorMessage: ErrorMessage) {
    super(errorMessage);
  }
}
