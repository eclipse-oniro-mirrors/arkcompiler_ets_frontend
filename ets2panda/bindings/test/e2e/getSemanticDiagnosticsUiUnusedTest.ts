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

import { LspDiagnosticNode } from '../../src/lsp';
import { getLspWithUi, getRealPath } from '../utils';

describe('getSemanticDiagnosticsUiUnusedTest', () => {
  const moduleName: string = 'getSemanticDiagnosticsUiUnused';

  function countMessage(diagnostics: LspDiagnosticNode[], message: string): number {
    return diagnostics.filter((diagnostic) => diagnostic.message === message).length;
  }

  function expectUnusedWarnings(diagnostics: LspDiagnosticNode[], expectedMessages: string[]): void {
    expectedMessages.forEach((message) => {
      const diagnostic = diagnostics.find((item) => item.message === message);
      expect(diagnostic).toBeDefined();
      expect(diagnostic).toMatchObject({
        message,
        severity: 2,
        data: 'unusedSymbol'
      });
      expect(countMessage(diagnostics, message)).toBe(1);
    });
  }

  function expectNoWarnings(diagnostics: LspDiagnosticNode[], unexpectedMessages: string[]): void {
    unexpectedMessages.forEach((message) => {
      const count = countMessage(diagnostics, message);
      if (count !== 0) {
        throw new Error(`Unexpected unused warning: ${message}`);
      }
    });
  }

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);

    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getSemanticDiagnostics_ui_unused_warning', () => {
      const diagnostics =
        getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnosticsUiUnused.ets'))?.diagnostics ??
        [];
      expectUnusedWarnings(diagnostics, [`'unusedUiLocal' is never used`]);
      expectNoWarnings(diagnostics, [
        `'Column' is never used`,
        `'Text' is never used`,
        `'UnusedWarningComponent' is never used`,
        `'message' is never used`,
        `'makeMessage' is never used`
      ]);
    });

    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getSemanticDiagnostics_kit_ui_unused_warning', () => {
      const diagnostics =
        getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'getSemanticDiagnosticsKitUiUnused.ets'))
          ?.diagnostics ?? [];

      expectUnusedWarnings(diagnostics, [`'unusedKitUiLocal' is never used`]);
      expectNoWarnings(diagnostics, [
        `'Row' is never used`,
        `'Text' is never used`,
        `'KitUnusedWarningComponent' is never used`,
        `'title' is never used`,
        `'formatTitle' is never used`
      ]);
    });
  });
});
