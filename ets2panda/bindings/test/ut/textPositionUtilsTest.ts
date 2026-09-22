/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import { TextPositionUtils } from '../../src/lsp/utils';

describe('TextPositionUtils UTF-16 and code point offsets', () => {
  const source = 'a😀中b';

  test('converts UTF-16 offsets to code point offsets', () => {
    expect(TextPositionUtils.utf16OffsetToCodePointOffset(source, 0)).toBe(0);
    expect(TextPositionUtils.utf16OffsetToCodePointOffset(source, 1)).toBe(1);
    expect(TextPositionUtils.utf16OffsetToCodePointOffset(source, 3)).toBe(2);
    expect(TextPositionUtils.utf16OffsetToCodePointOffset(source, 4)).toBe(3);
    expect(TextPositionUtils.utf16OffsetToCodePointOffset(source, 5)).toBe(4);
  });

  test('converts code point offsets to UTF-16 offsets', () => {
    expect(TextPositionUtils.codePointOffsetToUtf16Offset(source, 0)).toBe(0);
    expect(TextPositionUtils.codePointOffsetToUtf16Offset(source, 1)).toBe(1);
    expect(TextPositionUtils.codePointOffsetToUtf16Offset(source, 2)).toBe(3);
    expect(TextPositionUtils.codePointOffsetToUtf16Offset(source, 3)).toBe(4);
    expect(TextPositionUtils.codePointOffsetToUtf16Offset(source, 4)).toBe(5);
  });

  test('converts one-based code point columns to UTF-16 columns', () => {
    const multilineSource = 'plain\na😀中b\n';

    expect(TextPositionUtils.codePointColumnToUtf16Column(multilineSource, 2, 1)).toBe(1);
    expect(TextPositionUtils.codePointColumnToUtf16Column(multilineSource, 2, 3)).toBe(4);
    expect(TextPositionUtils.codePointColumnToUtf16Column(multilineSource, 2, 5)).toBe(6);
  });
});
