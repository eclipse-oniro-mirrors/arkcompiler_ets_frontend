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

describe('getOrganizeImportsTest', () => {
  const moduleName: string = 'getOrganizeImports';
  const EXPECT_000 = {
    fileName: getRealPath(moduleName, 'getOrganizeImports1.ets'),
    textChanges: [
      {
        span: { start: 608, length: 306 },
        newText: `import { Entry, Component } from '@ohos.arkui.component';\nimport { State } from '@ohos.arkui.stateManagement';\n\nimport { B, C } from './getOrganizeImports2';\n\n`
      }
    ]
  };
  const EXPECT_001 = {
    fileName: getRealPath(moduleName, 'ExtractDefaultImport1_import.ets'),
    textChanges: [
      {
        span: { start: 608, length: 56 },
        newText: `import Foo, { one } from './ExtractDefaultImport1_export';`
      }
    ]
  };
  const EXPECT_002 = {
    fileName: getRealPath(moduleName, 'ExtractDefaultImport2_import.ets'),
    textChanges: [
      {
        span: { start: 608, length: 51 },
        newText: `import Foo from './ExtractDefaultImport2_export';`
      }
    ]
  };
  const lsp = getLsp(moduleName);
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getOrganizeImports_000', () => {
    const res = lsp.getOrganizeImports(getRealPath(moduleName, 'getOrganizeImports1.ets'));
    expect(res?.fileTextChanges.length).toBe(1);
    expect(res?.fileTextChanges[0]).toMatchObject(EXPECT_000);
  });
  test('getOrganizeImports_001', () => {
    const res = lsp.getOrganizeImports(getRealPath(moduleName, 'ExtractDefaultImport1_import.ets'));
    expect(res?.fileTextChanges.length).toBe(1);
    expect(res?.fileTextChanges[0]).toMatchObject(EXPECT_001);
  });
  test('getOrganizeImports_002', () => {
    const res = lsp.getOrganizeImports(getRealPath(moduleName, 'ExtractDefaultImport2_import.ets'));
    expect(res?.fileTextChanges.length).toBe(1);
    expect(res?.fileTextChanges[0]).toMatchObject(EXPECT_002);
  });
});
