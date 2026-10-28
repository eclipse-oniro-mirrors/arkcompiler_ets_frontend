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

import {
  GlueGenDiagnosticError,
  GlueGenError,
  GlueGenErrorCode,
  GlueGenErrorList,
  GlueGenInternalError,
  errorMessage,
} from '../../errors';
import { LogData } from '../../logger';
import type { GlueGenContext } from '../../pipeline/context';
import type { StageScope } from '../../pipeline';
import type { ModuleTable } from './moduleTable';

interface ResolveModulesInput {
  readonly 'resolve-modules': ModuleTable;
}

export async function resolveInteropConfig(
  _scope: StageScope<GlueGenContext, readonly []>,
  inputs: ResolveModulesInput,
): Promise<ReadonlyMap<string, common.interopConfig.InteropTarget>> {
  try {
    // The shared resolver validates the configuration format plus every static
    // and dynamic entry; gluegen consumes only the static files.
    return await common.interopConfig.resolveInteropConfig(inputs['resolve-modules']);
  } catch (error) {
    throw toGlueGenError(error);
  }
}

/** Maps the shared resolver's errors onto gluegen's diagnostic types. */
function toGlueGenError(error: unknown): GlueGenError | GlueGenErrorList {
  if (error instanceof GlueGenError) {
    return error;
  }
  if (error instanceof common.errors.AggregateUserError) {
    return new GlueGenErrorList(
      error.errors.flatMap((failure) => {
        const mapped = toGlueGenError(failure);
        return mapped instanceof GlueGenErrorList ? mapped.errors : [mapped];
      }),
    );
  }
  if (error instanceof common.errors.UserError) {
    return new GlueGenDiagnosticError(
      new LogData({
        ...common.interopConfig.toHvigorErrorMessage(error.errorMessage),
        code: common.interopConfig.INTEROP_CONFIG_ERROR_CODE,
      }),
    );
  }
  return new GlueGenInternalError(
    new LogData({
      code: GlueGenErrorCode.INTERNAL_FAILURE,
      description: 'Gluegen could not resolve the module interop configuration.',
      cause: errorMessage(error, 'unknown interop configuration failure'),
    }),
  );
}
