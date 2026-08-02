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

import type { GlueGenReport } from '../../src/contracts';
import { LogData } from '../../src/logger';
import { NativeExitCode } from '../../src/native';
import { parseGlueGenReport } from '../../src/native/report';

describe('native report contract', () => {
  it('restores diagnostic entries as LogData', () => {
    const report: GlueGenReport = {
      diagnostics: {
        errors: [
          new LogData({
            code: '11420050',
            description: 'Native generation failed.',
            cause: 'Unsupported interop declaration.',
            position: 'Index.ets',
            solutions: ['Update the declaration.'],
            moreInfo: { phase: 'generation' },
          }),
        ],
        warnings: [
          new LogData({
            code: '11420049',
            description: 'Native analysis warning.',
          }),
        ],
      },
    };

    const parsed = parseGlueGenReport(JSON.stringify(report));

    expect(parsed).toEqual(report);
    expect(parsed.diagnostics.errors[0]).toBeInstanceOf(LogData);
    expect(parsed.diagnostics.warnings[0]).toBeInstanceOf(LogData);
    expect(NativeExitCode.Success).toBe(0);
    expect(NativeExitCode.DiagnosticError).toBe(1);
    expect(NativeExitCode.InternalError).toBe(2);
  });

  it.each([
    ['non-JSON input', '{invalid-json'],
    ['a missing diagnostics section', JSON.stringify({})],
    ['a non-object diagnostics section', JSON.stringify({ diagnostics: [] })],
    ['missing errors', JSON.stringify({ diagnostics: { warnings: [] } })],
    ['missing warnings', JSON.stringify({ diagnostics: { errors: [] } })],
    ['non-array errors', JSON.stringify({ diagnostics: { errors: {}, warnings: [] } })],
    ['non-array warnings', JSON.stringify({ diagnostics: { errors: [], warnings: {} } })],
    [
      'an error without a code',
      JSON.stringify({
        diagnostics: {
          errors: [{ description: 'Missing code.' }],
          warnings: [],
        },
      }),
    ],
    [
      'a warning without a description',
      JSON.stringify({
        diagnostics: {
          errors: [],
          warnings: [{ code: '11420049' }],
        },
      }),
    ],
    [
      'an invalid LogData field',
      JSON.stringify({
        diagnostics: {
          errors: [{ code: '11420050', description: 'Failure.', solutions: 'fix it' }],
          warnings: [],
        },
      }),
    ],
    [
      'an extra LogData field',
      JSON.stringify({
        diagnostics: {
          errors: [{ code: '11420050', description: 'Failure.', extra: true }],
          warnings: [],
        },
      }),
    ],
    [
      'an extra top-level field',
      JSON.stringify({
        diagnostics: { errors: [], warnings: [] },
        extra: true,
      }),
    ],
    [
      'an extra diagnostics field',
      JSON.stringify({
        diagnostics: { errors: [], warnings: [], extra: true },
      }),
    ],
  ])('rejects %s', (_description, serialized) => {
    expect(() => parseGlueGenReport(serialized)).toThrow();
  });
});
