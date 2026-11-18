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
import path from 'path';
import { getMultiModuleLsp, getRealPath } from '../utils';
import { ModuleDescriptor } from '../../src/lsp/generateBuildConfig';

const projectName = 'dynamicDeclRefresh';
// The module name is kept unique across the e2e fixtures: the static import content cache on
// the native side is process-global, and a worker process reused across test files would
// otherwise serve this suite the 'har'-keyed entries cached by another fixture.
const declModuleRoot = path.resolve('test/testcases', projectName, 'interopdecl');
// The separated interop context task layout - the resolveDeclFilesPath candidate that also
// covers real hvigor outputs for this manifest.
const buildDeclDir = path.join(
  declModuleRoot,
  'build',
  'default',
  'intermediates',
  'interop_context',
  'default'
);
const declsDir = path.join(buildDeclDir, 'decls');
const declPath = path.join(declsDir, 'Index.d.ets');

// The build system emits the dynamic module decl file info with absolute paths; it is
// materialized here before the LSP is constructed, mirroring a built project. The exported
// name is a parameter so the suite can simulate a declaration regeneration (rename).
function writeDeclFiles(exportName: string): void {
  fs.mkdirSync(declsDir, { recursive: true });
  fs.writeFileSync(
    declPath,
    `declare class ${exportName} {\n    static val: number;\n}\nexport { ${exportName} };\n`
  );
  fs.writeFileSync(
    path.join(buildDeclDir, 'decl-fileInfo.json'),
    JSON.stringify(
      {
        packageName: 'interopdecl',
        files: {
          Index: {
            declPath,
            filePath: path.join(declModuleRoot, 'Index.ets'),
            ohmUrl: ''
          }
        }
      },
      null,
      2
    )
  );
}

writeDeclFiles('HarHelper');

describe('dynamicDeclRefreshTest', () => {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'interopdecl', moduleType: 'har', srcPath: 'interopdecl' }
  ];
  let lsp: ReturnType<typeof getMultiModuleLsp> | undefined;
  const getLspInstance = (): ReturnType<typeof getMultiModuleLsp> => {
    if (lsp === undefined) {
      lsp = getMultiModuleLsp(projectName, modules, []);
      lsp.initAstCache();
    }
    return lsp;
  };

  afterAll(() => {
    fs.rmSync(path.join(declModuleRoot, 'build'), { recursive: true, force: true });
  });

  // The refresh flow needs the native 'refreshExternalProgram' capability: a declaration edit
  // is routed to RefreshExternalProgram instead of a regular active-file recompile. Older
  // prebuilt libraries keep the previous (stale) behavior; the cases return early there.
  function supportsRefresh(): boolean {
    return getLspInstance().hasExternalProgramRefreshSupport();
  }

  function consumerDiagnostics(): string[] {
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const diagnostics = getLspInstance().getSemanticDiagnostics(importerFile)?.diagnostics ?? [];
    return diagnostics.map((diagnostic) => String(diagnostic.message));
  }

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('decl file change refreshes the consumer diagnostics', () => {
    if (!supportsRefresh()) {
      return;
    }
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const importerSource = fs.readFileSync(importerFile, 'utf8');

    // Make the consumer the active file: its import resolves through the declaration.
    getLspInstance().modifyFilesMap(importerFile, { newDoc: importerSource });
    expect(consumerDiagnostics().some((message) => message.includes('HarHelper'))).toBe(false);

    // Rename the export in the declaration, both on disk (declgen regeneration) and in the
    // editor buffer (didChange): the stale declaration must not survive anywhere.
    const renamedDecl = 'declare class RenamedHelper {\n    static val: number;\n}\nexport { RenamedHelper };\n';
    fs.writeFileSync(declPath, renamedDecl);
    getLspInstance().modifyFilesMap(declPath, { newDoc: renamedDecl });

    const messages = consumerDiagnostics();
    expect(messages.some((message) => message.includes('HarHelper'))).toBe(true);
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('decl file change refreshes the auto import suggestions', () => {
    if (!supportsRefresh()) {
      return;
    }
    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const source = fs.readFileSync(queryFile, 'utf8');

    // The stale export stops matching its trigger token and the regenerated one appears under
    // its own prefix: auto import entries are prefix-matched, so each name is queried at its
    // own token.
    const staleOffset = source.lastIndexOf('harH') + 'harH'.length;
    const staleRes = getLspInstance().getCompletionAtPosition(queryFile, staleOffset);
    const staleNames = (staleRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(staleNames).not.toContain('HarHelper');

    const freshOffset = source.lastIndexOf('renam') + 'renam'.length;
    const freshRes = getLspInstance().getCompletionAtPosition(queryFile, freshOffset);
    const freshNames = (freshRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(freshNames).toContain('RenamedHelper');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('newly added decl export reaches the consumer', () => {
    if (!supportsRefresh()) {
      return;
    }
    // Adding an export keeps the declaration file and every import key unchanged: only the
    // parsed program content changes, which is exactly what the refresh re-introduces.
    const currentDecl = fs.readFileSync(declPath, 'utf-8');
    const withNewExport = `${currentDecl}declare class Second {\n    static val: number;\n}\nexport { Second };\n`;
    fs.writeFileSync(declPath, withNewExport);
    getLspInstance().modifyFilesMap(declPath, { newDoc: withNewExport });

    // The consumer imports the newly added export through the module root: the entry
    // declaration now carries it, the import must resolve without an error.
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const importerSource = fs.readFileSync(importerFile, 'utf8');
    const updatedSource = importerSource.replace(
      "import { HarHelper } from 'interopdecl';",
      "import { RenamedHelper, Second } from 'interopdecl';"
    );
    getLspInstance().modifyFilesMap(importerFile, { newDoc: updatedSource });
    const messages = consumerDiagnostics();
    expect(messages.some((message) => message.includes('Second'))).toBe(false);

    // The auto import suggestions serve the added export as well.
    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const querySource = fs.readFileSync(queryFile, 'utf8');
    const offset = querySource.lastIndexOf('seco') + 'seco'.length;
    const res = getLspInstance().getCompletionAtPosition(queryFile, offset);
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(names).toContain('Second');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('decl refresh registers the exports when the active file does not import the module', () => {
    if (!supportsRefresh()) {
      return;
    }
    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const querySource = fs.readFileSync(queryFile, 'utf8');

    // The editor sits on a file that never imports the dynamic module: the consumer
    // recompile inside the refresh cannot re-introduce the declaration, the exports must
    // re-register through the refresh itself.
    getLspInstance().modifyFilesMap(queryFile, { newDoc: querySource });

    const currentDecl = fs.readFileSync(declPath, 'utf-8');
    const withNewExport = `${currentDecl}declare class Fourth {\n    static val: number;\n}\nexport { Fourth };\n`;
    fs.writeFileSync(declPath, withNewExport);
    getLspInstance().modifyFilesMap(declPath, { newDoc: withNewExport });

    // The added export is suggested: auto import entries are prefix-matched, so each name is
    // queried at its own token - the new export at 'fourt', an older one at 'seco'.
    const res = getLspInstance().getCompletionAtPosition(
      queryFile,
      querySource.lastIndexOf('fourt') + 'fourt'.length
    );
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(names).toContain('Fourth');

    const secondRes = getLspInstance().getCompletionAtPosition(
      queryFile,
      querySource.lastIndexOf('seco') + 'seco'.length
    );
    const secondNames = (secondRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(secondNames).toContain('Second');
  });
});
