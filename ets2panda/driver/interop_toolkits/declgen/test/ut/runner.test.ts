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

import * as common from '@interop-toolkits/common';

import { ErrorCode, SentinelNotConfiguredError } from '../../src/errors';
import { logger } from '../../src/logger';
import { reportDeclgenError } from '../../src/runner/runner';

function sentinelError(fileName: string): SentinelNotConfiguredError {
  return new SentinelNotConfiguredError({
    description: 'Failed to validate interop entries.',
    cause: `${fileName} is not configured as an interop entry.`,
    solutions: ['Add it into the interop configuration.'],
  });
}

describe('reportDeclgenError', () => {
  let printErrorSpy: jest.SpyInstance;

  beforeEach(() => {
    printErrorSpy = jest.spyOn(logger, 'printError').mockImplementation(() => {});
  });

  afterEach(() => {
    printErrorSpy.mockRestore();
  });

  it('prints every aggregated user error as an independent error block', () => {
    reportDeclgenError(new common.errors.AggregateUserError([sentinelError('a.ts'), sentinelError('b.ets')]));

    expect(printErrorSpy).toHaveBeenCalledTimes(2);
    const [firstData, secondData] = printErrorSpy.mock.calls.map((call) => call[0]);
    expect(firstData.code).toBe(ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG);
    expect(firstData.cause).toContain('a.ts');
    expect(secondData.code).toBe(ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG);
    expect(secondData.cause).toContain('b.ets');
  });

  it('dispatches per inner error type when the aggregate mixes user error types', () => {
    const interopConfigError = new common.interopConfig.InteropConfigError({
      description: 'The interop configuration for package "pkg" could not be read or parsed.',
      cause: 'unknown interop configuration failure',
      solutions: ['Check that the file exists and contains valid JSON5.'],
    });

    reportDeclgenError(new common.errors.AggregateUserError([interopConfigError, sentinelError('a.ts')]));

    expect(printErrorSpy).toHaveBeenCalledTimes(2);
    const [firstData, secondData] = printErrorSpy.mock.calls.map((call) => call[0]);
    expect(firstData.code).toBe(ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG);
    expect(firstData.cause).toContain('unknown interop configuration failure');
    expect(secondData.code).toBe(ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG);
    expect(secondData.cause).toContain('a.ts');
  });

  it('flattens nested aggregates through recursive dispatch', () => {
    const nested = new common.errors.AggregateUserError([sentinelError('a.ts')]);

    reportDeclgenError(new common.errors.AggregateUserError([nested, sentinelError('b.ets')]));

    expect(printErrorSpy).toHaveBeenCalledTimes(2);
    const causes = printErrorSpy.mock.calls.map((call) => call[0].cause);
    expect(causes[0]).toContain('a.ts');
    expect(causes[1]).toContain('b.ets');
  });

  it('prints a single sentinel error as one error block', () => {
    reportDeclgenError(sentinelError('a.ts'));

    expect(printErrorSpy).toHaveBeenCalledTimes(1);
    expect(printErrorSpy.mock.calls[0][0].code).toBe(ErrorCode.DECLGEN_INVALID_INTEROP_CONFIG);
  });

  it('prints an internal error with the internal error code', () => {
    reportDeclgenError(new common.errors.InternalError('unexpected failure'));

    expect(printErrorSpy).toHaveBeenCalledTimes(1);
    expect(printErrorSpy.mock.calls[0][0].code).toBe(ErrorCode.DECLGEN_INTERNAL_ERROR);
    expect(printErrorSpy.mock.calls[0][0].description).toBe('unexpected failure');
  });

  it('prints nothing for unknown error types', () => {
    reportDeclgenError(new Error('unknown'));

    expect(printErrorSpy).not.toHaveBeenCalled();
  });
});
