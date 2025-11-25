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

describe('getSpanOfEnclosingCommentTest', () => {
  const moduleName: string = 'getSpanOfEnclosingComment';
  const EXPECT_000 = {
    start: 0,
    length: 0
  };
  const EXPECT_001 = {
    start: 659,
    length: 6
  };
  const EXPECT_002 = {
    start: 659,
    length: 9
  };
  const lsp = getLsp(moduleName);
  test('getSpanOfEnclosingComment_000', () => {
    const res = lsp.getSpanOfEnclosingComment(getRealPath(moduleName, 'getSpanOfEnclosingComment1.ets'), 669, false);
    expect(res).toMatchObject(EXPECT_000);
  });
  test('getSpanOfEnclosingComment_001', () => {
    const res = lsp.getSpanOfEnclosingComment(getRealPath(moduleName, 'getSpanOfEnclosingComment1.ets'), 663, false);
    expect(res).toMatchObject(EXPECT_001);
  });
  test('getSpanOfEnclosingComment_002', () => {
    const res = lsp.getSpanOfEnclosingComment(getRealPath(moduleName, 'getSpanOfEnclosingComment2.ets'), 663, false);
    expect(res).toMatchObject(EXPECT_002);
  });
});
