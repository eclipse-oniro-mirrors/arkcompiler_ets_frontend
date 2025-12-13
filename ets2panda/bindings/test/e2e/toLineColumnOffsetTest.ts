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

describe('toLineColumnOffsetTest', () => {
  const moduleName: string = 'toLineColumnOffset';
  const EXPECT_000 = {
    line: 0,
    character: 0
  };
  const EXPECT_001 = {
    line: 17,
    character: 642
  };
  const EXPECT_002 = {
    line: 18,
    character: 708
  };
  const lsp = getLsp(moduleName);
  test('toLineColumnOffset_000', () => {
    const res = lsp.toLineColumnOffset(getRealPath(moduleName, 'toLineColumnOffset1.ets'), 0);
    expect(res).toMatchObject(EXPECT_000);
  });
  test('toLineColumnOffset_001', () => {
    const res = lsp.toLineColumnOffset(getRealPath(moduleName, 'toLineColumnOffset1.ets'), 642);
    expect(res).toMatchObject(EXPECT_001);
  });
  test('toLineColumnOffset_002', () => {
    const res = lsp.toLineColumnOffset(getRealPath(moduleName, 'toLineColumnOffset2.ets'), 709);
    expect(res).toMatchObject(EXPECT_002);
  });
});
