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
import * as common from '@interop-toolkits/common';

export enum ErrorCode {
  DECLGEN_INTERNAL_ERROR = '11430001',
  DECLGEN_INVALID_INTEROP_CONFIG = '11430002',
  STATIC_WORKER_TASK_TIMEOUT = '11438001',
  STATIC_WORKER_PROCESS_FAILED = '11438002',
}

export class SentinelNotConfiguredError extends common.errors.UserError {
  constructor(err: common.errors.ErrorMessage) {
    super(err);
  }
}
