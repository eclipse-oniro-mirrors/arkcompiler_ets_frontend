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

// ui-syntax rule is moved to after-check and can be enabled after adaptation
xdescribe('getCodeFixesAtPositionTest', () => {
  const moduleName: string = 'getCodeFixesAtPosition';
  const EXPECT_000 = [
    {
      changes: [
        {
          fileName: getRealPath(moduleName, 'getCodeFixesAtPosition1.ets'),
          textChanges: [
            {
              span: {
                start: 990,
                length: 6
              },
              newText: ''
            }
          ]
        }
      ],
      description: "Remove the duplicate 'Entry' annotation",
      fixName: 'Fix',
      fixId_: 'UI_PLUGIN_SUGGEST',
      fixAllDescription_: 'Fix All Description'
    },
    {
      changes: [
        {
          fileName: getRealPath(moduleName, 'getCodeFixesAtPosition1.ets'),
          textChanges: []
        }
      ],
      description: "Remove the duplicate 'Entry' annotation",
      fixName: 'Fix',
      fixId_: 'UI_PLUGIN_SUGGEST',
      fixAllDescription_: 'Fix All Description'
    }
  ];

  const PLUGIN_LIST: string[] = process.env.SKIP_UI_PLUGINS ? [] : ['ui-syntax-plugins', 'ui-plugins', 'memo-plugins'];
  const lsp = getLsp(moduleName, PLUGIN_LIST);
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCodeFixesAtPosition_000', () => {
    const res = lsp.getCodeFixesAtPosition(getRealPath(moduleName, 'getCodeFixesAtPosition1.ets'), 994, 995, [4000]);
    expect(res).toMatchObject(EXPECT_000);
  });
});
