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

describe('getSemanticDiagnosticsTest', () => {
  const moduleName: string = 'getSemanticDiagnostics';
  const DIAGNOSTICS_001 = [
    {
      message: `Type '"hello"' cannot be assigned to type 'Double'`,
      range: { start: { line: 16, character: 19 }, end: { line: 16, character: 26 } }
    },
    {
      message: `No matching call signature for add("1", Int)`,
      range: { start: { line: 20, character: 1 }, end: { line: 20, character: 4 } }
    },
    {
      message: "Type '\"1\"' is not compatible with type 'Double' at index 1",
      range: { start: { line: 20, character: 5 }, end: { line: 20, character: 8 } }
    }
  ];
  const DIAGNOSTICS_002 = [
    {
      message: `No matching call signature for push("123")`,
      range: { start: { line: 19, character: 1 }, end: { line: 19, character: 4 } }
    },
    {
      message: `Type '"123"' is not compatible with type 'Double' at index 1`,
      range: { start: { line: 19, character: 10 }, end: { line: 19, character: 15 } }
    }
  ];
  const lsp = getLsp(moduleName);
  test('getSemanticDiagnostics_000', () => {
    const res = lsp.getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics1.ets'));
    expect(res?.diagnostics).toStrictEqual([]);
  });
  test('getSemanticDiagnostics_001', () => {
    const res = lsp.getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics2.ets'));
    expect(res?.diagnostics.length).toBe(3);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_001);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getSemanticDiagnostics_002', () => {
    const res = lsp.getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics3.ets'));
    expect(res?.diagnostics.length).toBe(2);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_002);
  });
});
