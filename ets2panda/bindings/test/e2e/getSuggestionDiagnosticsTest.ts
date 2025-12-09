/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { getLsp, getRealPath } from '../utils';

describe('getSuggestionDiagnosticsTest', () => {
  const moduleName: string = 'getSuggestionDiagnostics';
  const DIAGNOSTICS_000 = [
    {
      message: 'This_may_be_converted_to_an_async_function',
      range: { start: { line: 16, character: 1 }, end: { line: 18, character: 3 } }
    },
    {
      message: 'This_may_be_converted_to_an_async_function',
      range: { start: { line: 16, character: 10 }, end: { line: 18, character: 3 } }
    },
    {
      message: 'This_may_be_converted_to_an_async_function',
      range: { start: { line: 19, character: 1 }, end: { line: 21, character: 2 } }
    },
    {
      message: 'This_may_be_converted_to_an_async_function',
      range: { start: { line: 19, character: 10 }, end: { line: 21, character: 2 } }
    }
  ];
  const lsp = getLsp(moduleName);
  test('getSuggestionDiagnostics_000', () => {
    const res = lsp.getSuggestionDiagnostics(getRealPath(moduleName, 'getSuggestionDiagnostics1.ets'));
    expect(res?.diagnostics.length).toBe(4);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_000);
  });
});
