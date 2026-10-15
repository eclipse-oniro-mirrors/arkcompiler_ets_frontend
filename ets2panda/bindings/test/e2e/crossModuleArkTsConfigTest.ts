/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import { ModuleDescriptor } from '../../src';
import { getMultiModuleLsp, getRealPath } from '../utils';

describe('crossModuleArkTsConfigTest', () => {
  test('query miss uses the target module ArkTSConfig', () => {
    const projectName = 'getQuickInfoAtPositionCrossModule';
    const modules: ModuleDescriptor[] = [
      { name: 'har', moduleType: 'har', srcPath: 'har' },
      { name: 'entry', moduleType: 'entry', srcPath: 'entry' }
    ];
    const lsp = getMultiModuleLsp(projectName, modules, []);
    const entryFile = getRealPath(projectName, 'entry/EntryQuickInfo.ets');

    try {
      lsp.initAstCache();
      const diagnostics = lsp.getSemanticDiagnostics(entryFile)?.diagnostics ?? [];

      expect(diagnostics.map((diagnostic) => diagnostic.code)).toStrictEqual([292743, 292743]);
      expect(diagnostics.map((diagnostic) => diagnostic.message.valueOf())).toStrictEqual([
        "'foo' is never used",
        "'value' is never used"
      ]);
    } finally {
      lsp.dispose();
    }
  });
});