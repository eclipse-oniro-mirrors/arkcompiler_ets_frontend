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

import { ErrorMessage, UserError } from '../errors';
import { LogData } from '../hvigorLogger';

/** Returns an Error message while preserving a caller-specific fallback for non-Error throws. */
export function errorMessage(error: unknown, fallback: string): string {
  return error instanceof Error ? error.message : fallback;
}

export const INTEROP_CONFIG_ERROR_CODE = '11420002';

/**
 * Adapts an error message for hvigor's logger: its `moreInfo` contract is the localized
 * `{ cn, en }` shape, so structured entries are replaced with empty localized slots.
 */
export function toHvigorErrorMessage(errorMessage: ErrorMessage): ErrorMessage {
  return { ...errorMessage, moreInfo: { cn: '', en: '' } };
}

export class InteropConfigError extends UserError {
  public constructor(errorMessage: ErrorMessage) {
    super(errorMessage);
  }

  public override logData(code: string): LogData {
    return new LogData({ ...toHvigorErrorMessage(this.errorMessage), code });
  }
}
