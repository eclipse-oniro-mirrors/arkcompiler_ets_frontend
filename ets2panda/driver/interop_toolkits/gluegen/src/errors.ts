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

import type { LogData } from './logger';

/** Returns an Error message while preserving a caller-specific fallback for non-Error throws. */
export function errorMessage(error: unknown, fallback: string): string {
  return error instanceof Error ? error.message : fallback;
}

export enum GlueGenErrorCode {
  INVALID_BUILD_CONFIG = '11420001',
  INVALID_INTEROP_CONFIG = '11420002',
  GENERATE_INTEROP_FILE_LIST_FAIL = '11420003',
  GENERATE_ARKTS_CONFIG_FAIL = '11420004',
  NATIVE_PROCESS_FAIL = '11420005',
  NATIVE_RESPONSE_FAIL = '11420006',
  INTERNAL_FAILURE = '11420007',
}

export class GlueGenError extends Error {
  public constructor(public readonly logData: LogData) {
    super(logData.description);
    this.name = new.target.name;
  }
}

export class GlueGenDiagnosticError extends GlueGenError {}

export class GlueGenInternalError extends GlueGenError {}

export class GlueGenErrorList extends Error {
  public constructor(public readonly errors: readonly GlueGenError[]) {
    super(errors.map((error) => error.message).join('\n'));
    this.name = new.target.name;
  }
}
