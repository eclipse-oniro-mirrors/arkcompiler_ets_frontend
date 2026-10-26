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

import fs from 'fs';
import { getLsp, getRealPath, getLspWithUi } from '../utils';
import path from 'path';

describe('getReferencesAtPositionTest', () => {
  const moduleName: string = 'getReferencesAtPosition';
  const REFERENCES_001 = [
    { fileName: 'getReferencesAtPosition2.ets', start: 635, length: 1 },
    { fileName: 'getReferencesAtPosition2.ets', start: 665, length: 1 },
    { fileName: 'getReferencesAtPosition3.ets', start: 617, length: 1 }, 
    { fileName: 'getReferencesAtPosition3.ets', start: 667, length: 1 }
  ];
  const REFERENCES_002 = [
    { fileName: 'getReferencesAtPosition5.ets', start: 617, length: 1 },
    { fileName: 'getReferencesAtPosition5.ets', start: 655, length: 1 }
  ];
  const REFERENCES_003 = [
    { fileName: 'getReferencesAtPosition6.ets', start: 695, length: 4 },
    { fileName: 'getReferencesAtPosition6.ets', start: 708, length: 4 }
  ];
  function expectReferences(references: any, expected: { fileName: string; start: number; length: number }) {
    references.fileName = path.basename(references.fileName);
    expect(references).toMatchObject(expected);
  }
  describe('No UI Plugins', () => {
    const lsp = getLsp(moduleName);
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition2.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition2.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition3.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition3.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition4.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition4.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition5.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition5.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition6.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition6.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition9.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition9.ets'), 'utf8') });
    lsp.modifyFilesMap(getRealPath(moduleName, 'getReferencesAtPosition10.ets'), { newDoc: fs.readFileSync(getRealPath(moduleName, 'getReferencesAtPosition10.ets'), 'utf8') });
    test('getReferencesAtPosition_000', () => {
      const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition1.ets'), 613);
      expect(res?.length).toBe(0);
    });
    test('getReferencesAtPosition_001', () => {
      const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition2.ets'), 635);
      expect(res?.length).toBe(4);
      const length = res ? res.length : 0;
      for (let i = 0; i < length; i++) {
        expectReferences(res ? res[i] : undefined, REFERENCES_001[i]);
      }
    });
    test('getReferencesAtPosition_002', () => {
      const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition4.ets'), 625);
      expect(res?.length).toBe(2);
      const length = res ? res.length : 0;
      for (let i = 0; i < length; i++) {
        expectReferences(res ? res[i] : undefined, REFERENCES_002[i]);
      }
    });
    test('getReferencesAtPosition_004_uses_utf16_offsets', () => {
      const fileName = getRealPath(moduleName, 'getReferencesAtPosition9.ets');
      const source = fs.readFileSync(fileName, 'utf8');
      const firstUse = source.indexOf('target++');
      const secondUse = source.lastIndexOf('target;');

      const res = lsp.getReferencesAtPosition(fileName, firstUse);

      expect(res?.map((reference) => reference.start).sort((left, right) => left - right))
        .toEqual([firstUse, secondUse]);
      expect(res?.every((reference) => reference.length === 'target'.length)).toBe(true);
    });

    test('getReferencesAtPosition_005_handles_line_column_offset_after_emoji', () => {
      const getOffsetFromLineAndColumn = (fileName: string, marker: string) => {
        const source = fs.readFileSync(fileName, 'utf8');
        const markerOffset = source.indexOf(marker);
        const prefix = source.slice(0, markerOffset);
        const line = prefix.split(String.fromCharCode(10)).length;
        const lineStart = source.lastIndexOf(String.fromCharCode(10), markerOffset - 1) + 1;
        const column = markerOffset - lineStart + 1;
        const offset = lsp.getOffsetByColAndLine(fileName, line, column);
        return { line, column, offset };
      };

      const asciiFileName = getRealPath(moduleName, 'getReferencesAtPosition2.ets');
      const asciiPosition = getOffsetFromLineAndColumn(
        asciiFileName,
        'a = 1'
      );
      const asciiReferences = lsp.getReferencesAtPosition(
        asciiFileName,
        asciiPosition.offset
      );
      expect(asciiReferences).toHaveLength(4);

      const unicodeFileName = getRealPath(moduleName, 'getReferencesAtPosition10.ets');
      const unicodeSource = fs.readFileSync(unicodeFileName, 'utf8');
      const firstUse = unicodeSource.indexOf('target++');
      const secondUse = unicodeSource.lastIndexOf('target;');
      const unicodePosition = getOffsetFromLineAndColumn(unicodeFileName, 'target++');
      const unicodeReferences = lsp.getReferencesAtPosition(unicodeFileName, unicodePosition.offset);

      expect(unicodeReferences?.map((reference) => reference.start).sort((left, right) => left - right))
        .toEqual([firstUse, secondUse]);
      expect(unicodePosition.offset).toBe(firstUse);
      expect(lsp.getColAndLineByOffset(unicodeFileName, unicodePosition.offset)).toMatchObject({
        line: unicodePosition.line,
        column: unicodePosition.column
      });

      const sameLineEmojiPosition = getOffsetFromLineAndColumn(unicodeFileName, '";');
      const sameLinePrefix = unicodeSource
        .slice(0, sameLineEmojiPosition.offset)
        .split(String.fromCharCode(10))
        .pop()!;
      const codePointColumn = Array.from(sameLinePrefix).length + 1;
      expect(sameLineEmojiPosition.column).toBe(codePointColumn + 2);
      expect(lsp.getColAndLineByOffset(unicodeFileName, sameLineEmojiPosition.offset)).toMatchObject({
        line: sameLineEmojiPosition.line,
        column: sameLineEmojiPosition.column
      });
    });
  });

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getReferencesAtPosition_003', () => {
      const res = getUiLsp().getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition6.ets'), 697);
      expect(res?.length).toBe(2);
      const length = res ? res.length : 0;
      for (let i = 0; i < length; i++) {
        expectReferences(res ? res[i] : undefined, REFERENCES_003[i]);
      }
    });
  });
});
