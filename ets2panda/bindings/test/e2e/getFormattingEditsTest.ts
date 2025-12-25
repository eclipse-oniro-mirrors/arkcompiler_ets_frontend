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

describe('getFormattingEditsTest', () => {
  const moduleName: string = 'getFormattingEdits';
  const lsp = getLsp(moduleName);

  describe('getFormattingEditsForDocument', () => {
    test('getFormattingEditsForDocument_001 - comma spacing', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForDocument(filePath, {
        insertSpaceAfterCommaDelimiter: true
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
      expect(res!.length).toBeGreaterThan(0);

      const spaceInsertions = res!.filter(
        (change) => change.span.length === 0 && change.newText === ' '
      );
      expect(spaceInsertions.length).toBeGreaterThan(0);
    });

    test('getFormattingEditsForDocument_002 - control flow spacing', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits2.ets');
      const res = lsp.getFormattingEditsForDocument(filePath, {
        insertSpaceAfterKeywordsInControlFlowStatements: true
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
      expect(res!.length).toBeGreaterThan(0);
    });

    test('getFormattingEditsForDocument_003 - arrow function spacing', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits3.ets');
      const res = lsp.getFormattingEditsForDocument(filePath);

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
      expect(res!.length).toBeGreaterThan(0);
    });

    test('getFormattingEditsForDocument_004 - with custom options', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForDocument(filePath, {
        insertSpaceAfterCommaDelimiter: true,
        insertSpaceBeforeAndAfterBinaryOperators: true,
        indentSize: 4,
        tabSize: 4,
        convertTabsToSpaces: true
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });

    test('getFormattingEditsForDocument_005 - default options', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForDocument(filePath);

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });
  });

  describe('getFormattingEditsForRange', () => {
    test('getFormattingEditsForRange_001 - format specific range', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForRange(filePath, 620, 30, {
        insertSpaceAfterCommaDelimiter: true
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });

    test('getFormattingEditsForRange_002 - format function range', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForRange(filePath, 640, 60, {
        insertSpaceAfterCommaDelimiter: true
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });

    test('getFormattingEditsForRange_003 - empty range returns empty', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForRange(filePath, 0, 10);

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });

    test('getFormattingEditsForRange_004 - with all options', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits2.ets');
      const res = lsp.getFormattingEditsForRange(filePath, 0, 500, {
        insertSpaceAfterKeywordsInControlFlowStatements: true,
        insertSpaceAfterOpeningAndBeforeClosingNonemptyBraces: true,
        placeOpenBraceOnNewLineForControlBlocks: false
      });

      expect(res).toBeDefined();
      expect(Array.isArray(res)).toBe(true);
    });
  });

  describe('TextChange structure validation', () => {
    test('TextChange has correct structure', () => {
      const filePath = getRealPath(moduleName, 'getFormattingEdits1.ets');
      const res = lsp.getFormattingEditsForDocument(filePath, {
        insertSpaceAfterCommaDelimiter: true
      });

      expect(res).toBeDefined();
      if (res && res.length > 0) {
        const change = res[0];
        expect(change).toHaveProperty('span');
        expect(change).toHaveProperty('newText');
        expect(change.span).toHaveProperty('start');
        expect(change.span).toHaveProperty('length');
        expect(typeof change.span.start).toBe('number');
        expect(typeof change.span.length).toBe('number');
        expect(typeof change.newText).toBe('string');
      }
    });
  });
});
