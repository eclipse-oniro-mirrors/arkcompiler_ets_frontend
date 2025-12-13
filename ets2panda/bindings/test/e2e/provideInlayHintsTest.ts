/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { getLsp, getRealPath } from '../utils';

describe('provideInlayHintsTest', () => {
  const moduleName: string = 'provideInlayHints';
  const EXPECT_000 = [
    {
      text: 'param1',
      number: 716,
      kind: 1,
      whitespaceBefore: false,
      whitespaceAfter: true
    },
    {
      text: 'param2',
      number: 720,
      kind: 1,
      whitespaceBefore: false,
      whitespaceAfter: true
    }
  ];
  const EXPECT_001 = [
    {
      text: 'item',
      number: 687,
      kind: 1,
      whitespaceBefore: false,
      whitespaceAfter: true
    }
  ];
  const lsp = getLsp(moduleName);
  test('provideInlayHints_000', () => {
    const res = lsp.provideInlayHints(getRealPath(moduleName, 'provideInlayHints1.ets'), {
      start: 712,
      length: 11
    });
    expect(res).toMatchObject(EXPECT_000);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('provideInlayHints_001', () => {
    const res = lsp.provideInlayHints(getRealPath(moduleName, 'provideInlayHints2.ets'), {
      start: 683,
      length: 5
    });
    expect(res).toMatchObject(EXPECT_001);
  });
});
