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

describe('getSyntacticDiagnosticsTest', () => {
  const moduleName: string = 'getSyntacticDiagnostics';
  const DIAGNOSTICS_001 = [
    {
      message: `Unexpected token 'add'.`,
      range: { start: { line: 16, character: 9 }, end: { line: 16, character: 12 } }
    },
    {
      message: `Unexpected token ':'.`,
      range: { start: { line: 16, character: 14 }, end: { line: 16, character: 15 } }
    },
    {
      message: `Unexpected token, expected ',' or ')'.`,
      range: { start: { line: 16, character: 14 }, end: { line: 16, character: 15 } }
    },
    {
      message: `Unexpected token 'number'.`,
      range: { start: { line: 16, character: 16 }, end: { line: 16, character: 22 } }
    },
    {
      message: `Unexpected token ','.`,
      range: { start: { line: 16, character: 22 }, end: { line: 16, character: 23 } }
    },
    {
      message: `Unexpected token 'b'.`,
      range: { start: { line: 16, character: 24 }, end: { line: 16, character: 24 } }
    },
    {
      message: `Label must be followed by a loop statement.`,
      range: { start: { line: 16, character: 27 }, end: { line: 16, character: 33 } }
    },
    {
      message: `Unexpected token ')'.`,
      range: { start: { line: 16, character: 33 }, end: { line: 16, character: 34 } }
    },
    {
      message: `Unexpected token '{'.`,
      range: { start: { line: 16, character: 35 }, end: { line: 18, character: 2 } }
    },
    {
      message: `return keyword should be used in function body.`,
      range: { start: { line: 17, character: 5 }, end: { line: 17, character: 18 } }
    }
  ];
  const DIAGNOSTICS_002 = [
    {
      message: `Unexpected token, expected 'from'.`,
      range: { start: { line: 16, character: 1 }, end: { line: 16, character: 52 } }
    }
  ];
  const DIAGNOSTICS_003 = [
    {
      message: `A function can only be decorated by the 'Builder'.`,
      range: { start: { line: 22, character: 2 }, end: { line: 22, character: 7 } }
    },
    {
      message: `The '@Track' annotation can decorate only member variables of a class.`,
      range: { start: { line: 19, character: 2 }, end: { line: 19, character: 7 } }
    },
    {
      message: `The '@Track' annotation can decorate only member variables of a class.`,
      range: { start: { line: 22, character: 2 }, end: { line: 22, character: 7 } }
    },
    {
      message: `The '@Track' annotation can decorate only member variables of a class.`,
      range: { start: { line: 27, character: 2 }, end: { line: 27, character: 7 } }
    },
    {
      message: `The '@Track' annotation can decorate only member variables of a class.`,
      range: { start: { line: 36, character: 6 }, end: { line: 36, character: 11 } }
    }
  ];
  const PLUGIN_LIST: string[] = process.env.SKIP_UI_PLUGINS ? [] : ['ui-syntax-plugins', 'ui-plugins', 'memo-plugins'];
  const lsp = getLsp(moduleName, PLUGIN_LIST);
  test('getSyntacticDiagnostics_000', () => {
    const res = lsp.getSyntacticDiagnostics(getRealPath(moduleName, 'getSyntacticDiagnostics1.ets'));
    expect(res?.diagnostics).toStrictEqual([]);
  });
  test('getSyntacticDiagnostics_001', () => {
    const res = lsp.getSyntacticDiagnostics(getRealPath(moduleName, 'getSyntacticDiagnostics2.ets'));
    expect(res?.diagnostics.length).toBe(10);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_001);
  });
  test('getSyntacticDiagnostics_002', () => {
    const res = lsp.getSyntacticDiagnostics(getRealPath(moduleName, 'getSyntacticDiagnostics3.ets'));
    expect(res?.diagnostics.length).toBe(1);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_002);
  });

  // ui-syntax rule is moved to after-check and can be enabled after adaptation
  (process.env.SKIP_UI_PLUGINS ? test.skip : xtest)('getSyntacticDiagnostics_003', () => {
    const res = lsp.getSyntacticDiagnostics(getRealPath(moduleName, 'getSyntacticDiagnostics4.ets'));
    expect(res?.diagnostics.length).toBe(5);
    expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_003);
  });
});
