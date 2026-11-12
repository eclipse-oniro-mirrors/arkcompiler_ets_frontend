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

import fs from 'fs';
import { getMultiModuleLsp, getRealPath } from '../utils';
import { ModuleDescriptor } from '../../src/lsp/generateBuildConfig';

const projectName = 'getSubpathImportFix';

describe('moduleSwitchPrepareTest', () => {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'library', moduleType: 'har', srcPath: 'library' }
  ];

  // Switching back to a module must not leave a bare context: createFileCache
  // compiles the file, but only the incremental prepare registers its dependency
  // programs into the context's external sources and prepares the checker's
  // export/import caches. Codefix re-derives that work on every query until an
  // edit runs the prepare, so the switch-back path has to run it too.
  test('switching back to a module incrementally prepares the new active context', () => {
    const lsp = getMultiModuleLsp(projectName, modules, []);
    const entryFile = getRealPath(projectName, 'entry/Page.ets');
    const libraryFile = getRealPath(projectName, 'library/utils/Foo.ets');
    const entrySource = fs.readFileSync(entryFile, 'utf8');
    const librarySource = fs.readFileSync(libraryFile, 'utf8');
    const offset = entrySource.indexOf('new Foo()') + 'new '.length;
    const diagnosticFiles = (lsp as unknown as { diagnosticFiles: Set<string> }).diagnosticFiles;

    try {
      lsp.initAstCache();
      // Work in the entry module, then jump to the library module and edit there:
      // the active cache switches to the library config.
      lsp.modifyFilesMap(entryFile, { newDoc: entrySource });
      lsp.modifyFilesMap(libraryFile, { newDoc: librarySource });

      // Switch back without changing content: the config-mismatch branch rebuilds
      // the entry context. The incremental prepare must run on it - a bare
      // createFileCache context does not cache the file's diagnostics.
      diagnosticFiles.delete(entryFile);
      lsp.modifyFilesMap(entryFile, { newDoc: entrySource });
      expect(diagnosticFiles.has(entryFile)).toBe(true);

      // Queries on the prepared context return the expected results.
      const quickInfo = lsp.getQuickInfoAtPosition(entryFile, entrySource.indexOf('useFoo') + 'use'.length);
      expect(quickInfo).toBeDefined();
      const res = lsp.getCodeFixesAtPosition(entryFile, offset, offset + 'Foo'.length, [1005]);
      const importFix = (res ?? []).find(
        (action) => String(action.description.valueOf()) === "Add import {Foo} from 'library/utils/Foo'"
      );
      expect(importFix).toBeDefined();
      expect(String(importFix!.changes[0].textChanges[0].newText)).toBe("import { Foo } from 'library/utils/Foo';\n");
    } finally {
      lsp.dispose();
    }
  });

  // The same bare-context issue applies when the switched-to module's file is
  // edited right away (changed content): the rebuilt context must also run the
  // incremental prepare, not only the unchanged-content switch-back.
  test('editing a file of another module incrementally prepares the new active context', () => {
    const lsp = getMultiModuleLsp(projectName, modules, []);
    const entryFile = getRealPath(projectName, 'entry/Page.ets');
    const libraryFile = getRealPath(projectName, 'library/utils/Foo.ets');
    const entrySource = fs.readFileSync(entryFile, 'utf8');
    const librarySource = fs.readFileSync(libraryFile, 'utf8');
    const offset = entrySource.indexOf('new Foo()') + 'new '.length;
    const diagnosticFiles = (lsp as unknown as { diagnosticFiles: Set<string> }).diagnosticFiles;

    try {
      lsp.initAstCache();
      lsp.modifyFilesMap(entryFile, { newDoc: entrySource });
      lsp.modifyFilesMap(libraryFile, { newDoc: librarySource });

      // Edit the entry file (changed content) while the library module is active:
      // the config-mismatch branch rebuilds and must prepare the new context.
      diagnosticFiles.delete(entryFile);
      lsp.modifyFilesMap(entryFile, { newDoc: entrySource + '\n' });
      expect(diagnosticFiles.has(entryFile)).toBe(true);

      const res = lsp.getCodeFixesAtPosition(entryFile, offset, offset + 'Foo'.length, [1005]);
      const importFix = (res ?? []).find(
        (action) => String(action.description.valueOf()) === "Add import {Foo} from 'library/utils/Foo'"
      );
      expect(importFix).toBeDefined();
    } finally {
      lsp.dispose();
    }
  });
});
