/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
import fs from "fs"
import { getLspWithUi, getRealPath } from '../utils';

describe('getCodeFixesAtPositionTest', () => {
  const moduleName: string = 'getCodeFixesAtPosition';
  const EXPECT_000 = [
    {
      changes: [
        {
          fileName: getRealPath(moduleName, 'getCodeFixesAtPosition1.ets'),
          textChanges: [
            {
              span: {
                start: 996,
                length: 6
              },
              newText: ''
            }
          ]
        }
      ],
      description: "Remove the duplicate '@Entry' annotation.",
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
      description: "Remove the duplicate '@Entry' annotation.",
      fixName: 'Fix',
      fixId_: 'UI_PLUGIN_SUGGEST',
      fixAllDescription_: 'Fix All Description'
    }
  ];

  describe('No UI Plugins', () => {});

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCodeFixesAtPosition_000', () => {
      const file = getRealPath(moduleName, 'getCodeFixesAtPosition1.ets');
      const lsp = getUiLsp();
      lsp.initAstCache();
      lsp.modifyFilesMap(file, { newDoc: `${fs.readFileSync(file, 'utf8')}\n` });

      const res = lsp.getCodeFixesAtPosition(file, 999, 1000, [4000]);

      expect(res).toMatchObject(EXPECT_000);
    });

    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCodeFixesAtPosition_001_uses_utf16_offsets', () => {
      const file = getRealPath(moduleName, 'getCodeFixesAtPosition1.ets');
      const originalSource = fs.readFileSync(file, 'utf8');
      const duplicateEntry = originalSource.lastIndexOf('@Entry');
      const emojiComment = '// 😀😀😀😀😀😀😀😀😀\n';
      const source = originalSource.slice(0, duplicateEntry) + emojiComment + originalSource.slice(duplicateEntry);
      const entryStart = source.lastIndexOf('@Entry');
      const lsp = getUiLsp();
      lsp.initAstCache();
      lsp.modifyFilesMap(file, { newDoc: source });

      const res = lsp.getCodeFixesAtPosition(file, entryStart + 3, entryStart + 4, [4000]);
      const removal = res?.flatMap((action) => action.changes)
        .flatMap((change) => change.textChanges)
        .find((change) => change.newText.length === 0);

      expect(duplicateEntry).not.toBe(-1);
      expect(removal?.span.start).toBe(entryStart);
      expect(removal?.span.length).toBe('@Entry'.length);
    });
  });
});
