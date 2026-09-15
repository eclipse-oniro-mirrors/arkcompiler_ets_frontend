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

import fs from 'fs';
import { LspDiagnosticNode } from '../../src/lsp';
import { DEFAULT_PATH_CONFIG, UI_PLUGIN_LIST, getMultiModuleLsp, getRealPath } from '../utils';

interface ExpectedRange {
  start: { line: number; character: number };
  end: { line: number; character: number };
}

function formatDiagnostics(diagnostics: LspDiagnosticNode[]): string {
  return diagnostics
    .map((diagnostic) => `${diagnostic.message} ${JSON.stringify(diagnostic.range)}`)
    .join('\n');
}

describe('uiPluginImportPrefixDiagnosticsTest', () => {
  const moduleName = 'uiPluginImportPrefixDiagnostics';
  const fileName = 'importPrefixRange.ets';
  const importPath = 'har/src/main/ets/components/MainPage';

  function getImportPathRange(filePath: string): ExpectedRange {
    const lines = fs.readFileSync(filePath, 'utf-8').split('\n');
    const lineIndex = lines.findIndex((line) => line.includes(`'${importPath}'`));
    expect(lineIndex).toBeGreaterThanOrEqual(0);

    const quoteStart = lines[lineIndex].indexOf(`'${importPath}'`);
    const literalLength = importPath.length + 2;
    return {
      start: { line: lineIndex + 1, character: quoteStart + 1 },
      end: { line: lineIndex + 1, character: quoteStart + literalLength + 1 }
    };
  }

  function findHarPrefixDiagnostic(diagnostics: LspDiagnosticNode[]): LspDiagnosticNode | undefined {
    return diagnostics.find((diagnostic) => diagnostic.message.includes(`Can't find prefix for '${importPath}'`));
  }

  describe('With UI Plugins', () => {
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('import prefix diagnostic range stays on import string', () => {
      const filePath = getRealPath(moduleName, fileName);
      const buildSdkPath = process.env.BUILD_SDK_PATH ?? DEFAULT_PATH_CONFIG.buildSdkPath;
      const lsp = getMultiModuleLsp(
        moduleName,
        [{ name: moduleName, moduleType: 'har', srcPath: '.' }],
        UI_PLUGIN_LIST,
        { buildSdkPath }
      );
      const res = lsp.getSemanticDiagnostics(filePath);
      const diagnostics = res?.diagnostics ?? [];
      const diagnostic = findHarPrefixDiagnostic(diagnostics);

      if (!diagnostic) {
        throw new Error(`Missing target HAR prefix diagnostic. All diagnostics:\n${formatDiagnostics(diagnostics)}`);
      }
      expect(diagnostic.range).toMatchObject(getImportPathRange(filePath));
    });
  });
});
