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

describe('getRenameInfoTest', () => {
  const moduleName: string = 'getRenameInfo';
  const EXPECT_000 = {
    canRenameSuccess: true,
    fileToRename: '',
    kind: 'property',
    displayName: 'aaa',
    fullDisplayName: 'aaa',
    kindModifiers: '',
    triggerSpan: {
      start: 613,
      length: 3
    }
  };
  const EXPECT_001 = {
    canRenameFailure: false,
    localizedErrorMessage: 'You cannot rename this element'
  };
  const EXPECT_002 = {
    canRenameFailure: false,
    localizedErrorMessage: 'You cannot rename this element'
  };
  const lsp = getLsp(moduleName);
  test('getRenameInfo_000', () => {
    const res = lsp.getRenameInfo(getRealPath(moduleName, 'getRenameInfo1.ets'), 615);
    expect(res).toMatchObject(EXPECT_000);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getRenameInfo_001', () => {
    const res = lsp.getRenameInfo(getRealPath(moduleName, 'getRenameInfo2.ets'), 626);
    expect(res).toMatchObject(EXPECT_001);
  });
  test('getRenameInfo_002', () => {
    const res = lsp.getRenameInfo(getRealPath(moduleName, 'getRenameInfo3.ets'), 697);
    expect(res).toMatchObject(EXPECT_002);
  });
});
