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

import { LspDiagnosticNode } from '../../src/lsp';
import { getLsp, getLspWithUi, getRealPath } from '../utils';

describe('getSemanticDiagnosticsTest', () => {
  const moduleName: string = 'getSemanticDiagnostics';
  const DIAGNOSTICS_001 = [
    {
      message: `Type '"hello"' cannot be assigned to type 'Double'`,
      range: { start: { line: 16, character: 19 }, end: { line: 16, character: 26 } }
    },
    {
      message: `No matching call signature for add("1", Int)`,
      range: { start: { line: 20, character: 1 }, end: { line: 20, character: 4 } }
    },
    {
      message: "Type '\"1\"' is not compatible with type 'Double' at index 1",
      range: { start: { line: 20, character: 5 }, end: { line: 20, character: 8 } }
    }
  ];
  const DIAGNOSTICS_002 = [
    {
      message: `No matching call signature for push("123")`,
      range: { start: { line: 19, character: 1 }, end: { line: 19, character: 4 } }
    },
    {
      message: `Type '"123"' is not compatible with type 'Double' at index 1`,
      range: { start: { line: 19, character: 10 }, end: { line: 19, character: 15 } }
    }
  ];

  const UNUSED_WARNING_MESSAGES = [
    `'UnusedModule' is never used`,
    `'unusedTopLevelValue' is never used`,
    `'unusedTopLevelArrow' is never used`,
    `'UnusedClass' is never used`,
    `'unusedFunction' is never used`,
    `'UnusedEnum' is never used`,
    `'UnusedInterface' is never used`,
    `'UnusedTypeAlias' is never used`,
    `'DerivedFromUsedAsParentInterface' is never used`,
    `'unusedParam' is never used`,
    `'writtenOnlyParam' is never used`,
    `'unusedRest' is never used`,
    `'unusedLocal' is never used`,
    `'writtenOnlyLocal' is never used`,
    `'unusedFor' is never used`,
    `'unusedField' is never used`,
    `'writtenOnlyField' is never used`,
    `'unusedMethod' is never used`,
    `'value' is never used`,
    `'helper' is never used`,
    `'NamespaceA' is never used`,
    `'usedByLocalInitializer' is never used`,
    `'NamespaceB' is never used`
  ];
  const USED_SYMBOL_MESSAGES = [
    `'UsedImport' is never used`,
    `'exportedTopLevelArrow' is never used`,
    `'UsedAsBaseClass' is never used`,
    `'DerivedFromUsedAsBaseClass' is never used`,
    `'UsedAsParentInterface' is never used`,
    `'UsedTypeAlias' is never used`,
    `'usesTypeAlias' is never used`,
    `'localParameterAndForCases' is never used`,
    `'PrivateMemberCases' is never used`,
    `'SamePrivateFieldA' is never used`,
    `'SamePrivateFieldB' is never used`,
    `'SamePrivateMethodA' is never used`,
    `'SamePrivateMethodB' is never used`,
    `'useImport' is never used`,
    `'useDeclarations' is never used`
  ];

  function expectUnusedWarnings(diagnostics: LspDiagnosticNode[], expectedMessages: string[]): void {
    expectedMessages.forEach((message) => {
      const diagnostic = diagnostics.find((item) => item.message === message);
      expect(diagnostic).toBeDefined();
      expect(diagnostic).toMatchObject({
        message,
        severity: 2,
        data: 'unusedSymbol'
      });
    });
  }

  describe('No UI Plugins', () => {
    const lsp = getLsp(moduleName);
    test('getSemanticDiagnostics_000', () => {
      const res = lsp.getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics1.ets'));
      expect(res?.diagnostics).toStrictEqual([]);
    });
    test('getSemanticDiagnostics_001', () => {
      const res = lsp.getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics2.ets'));
      expect(res?.diagnostics.length).toBe(3);
      expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_001);
    });

    test('getSemanticDiagnostics_unused_warnings', () => {
      const filePath = getRealPath(moduleName, 'getSemanticDiagnosticsUnused.ets');
      const diagnostics = lsp.getSemanticDiagnostics(filePath)?.diagnostics ?? [];
      const messages = diagnostics.map((diagnostic) => diagnostic.message);

      expectUnusedWarnings(diagnostics, UNUSED_WARNING_MESSAGES);
      USED_SYMBOL_MESSAGES.forEach((message) => {
        expect(messages).not.toContain(message);
      });
    });
  });

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getSemanticDiagnostics_002', () => {
      const res = getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnostics3.ets'));
      expect(res?.diagnostics.length).toBe(2);
      expect(res?.diagnostics).toMatchObject(DIAGNOSTICS_002);
    });
  });
});
