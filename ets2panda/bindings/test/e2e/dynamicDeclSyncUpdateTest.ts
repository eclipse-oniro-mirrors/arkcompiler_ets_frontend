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

const projectName = 'dynamicDeclSyncUpdate';
// The module name is kept unique across the e2e fixtures: the static import content cache on
// the native side is process-global, and a worker process reused across test files would
// otherwise serve this suite the entries cached by another fixture.
const declModuleRoot = path.resolve('test/testcases', projectName, 'syncdecl');
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
// materialized here before the LSP is constructed, mirroring a built project.
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
        packageName: 'syncdecl',
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

describe('dynamicDeclSyncUpdateTest', () => {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'syncdecl', moduleType: 'har', srcPath: 'syncdecl' }
  ];
  const projectPath = path.resolve('test/testcases', projectName);
  const resolvedModules = modules.map((module) => ({ ...module, srcPath: path.resolve(projectPath, module.srcPath) }));

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

  // The sync flow needs the native 'refreshExternalProgram' capability; older prebuilt
  // libraries keep the previous behavior and the cases return early there.
  function supportsSyncRefresh(): boolean {
    return getLspInstance().hasExternalProgramRefreshSupport();
  }

  function consumerDiagnostics(): string[] {
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const diagnostics = getLspInstance().getSemanticDiagnostics(importerFile)?.diagnostics ?? [];
    return diagnostics.map((diagnostic) => String(diagnostic.message));
  }

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('sync after a fresh decl generation resolves the new entry', () => {
    if (!supportsSyncRefresh()) {
      return;
    }
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const importerSource = fs.readFileSync(importerFile, 'utf8');

    // The consumer is open and active: its context resolved the current declaration set.
    getLspInstance().modifyFilesMap(importerFile, { newDoc: importerSource });
    expect(consumerDiagnostics().some((message) => message.includes('HarHelper'))).toBe(false);

    // The declgen task has generated a brand new declaration the interop-context task has not
    // recorded yet (the manifest on disk still lists only Index). The IDE notices the fresh
    // declaration and triggers the sync.
    fs.writeFileSync(
      path.join(declsDir, 'New.d.ets'),
      'declare class BrandNew {\n    static val: number;\n}\nexport { BrandNew };\n'
    );
    getLspInstance().update(resolvedModules);

    // The consumer starts using the new entry through a plain query chain - the config the
    // active context parsed at creation no longer matches the file on disk, so the sync must
    // have rebuilt the context and pushed the fresh declaration content. The new declaration
    // is a sibling file, imported through its file-level key.
    const updatedSource = importerSource.replace(
      "import { HarHelper } from 'syncdecl';",
      "import { HarHelper } from 'syncdecl';\nimport { BrandNew } from 'syncdecl/New';"
    );
    getLspInstance().modifyFilesMap(importerFile, { newDoc: updatedSource });
    const messages = consumerDiagnostics();
    // The failure mode must not hide behind an unresolvable prefix error: both the symbol and
    // its import specifier have to resolve.
    expect(messages.some((message) => message.includes('BrandNew') || message.includes('syncdecl/New'))).toBe(
      false
    );

    // The auto import suggestions serve the new export as well.
    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const querySource = fs.readFileSync(queryFile, 'utf8');
    const offset = querySource.lastIndexOf('brand') + 'brand'.length;
    const res = getLspInstance().getCompletionAtPosition(queryFile, offset);
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(names).toContain('BrandNew');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('sync after an added export reaches the consumer', () => {
    if (!supportsSyncRefresh()) {
      return;
    }
    // Adding an export keeps the manifest key set and therefore the arktsconfig unchanged:
    // no context rebuild happens, the refresh must come through the declaration content
    // sync alone.
    const currentDecl = fs.readFileSync(declPath, 'utf-8');
    const withNewExport = `${currentDecl}declare class Third {\n    static val: number;\n}\nexport { Third };\n`;
    fs.writeFileSync(declPath, withNewExport);
    getLspInstance().update(resolvedModules);

    // The consumer imports and uses the newly added export: neither the symbol nor the
    // module root prefix may fail to resolve (and the used symbol stays clear of the
    // unused-import warning).
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const importerSource = fs.readFileSync(importerFile, 'utf8');
    const updatedSource = importerSource
      .replace(
        "import { HarHelper } from 'syncdecl';",
        "import { HarHelper, Third } from 'syncdecl';"
      )
      .replace('return HarHelper.val;', 'return HarHelper.val + Third.val;');
    getLspInstance().modifyFilesMap(importerFile, { newDoc: updatedSource });
    const messages = consumerDiagnostics();
    expect(messages.some((message) => message.includes('Third') || message.includes('syncdecl'))).toBe(false);

    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const querySource = fs.readFileSync(queryFile, 'utf8');
    const offset = querySource.lastIndexOf('thir') + 'thir'.length;
    const res = getLspInstance().getCompletionAtPosition(queryFile, offset);
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(names).toContain('Third');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('a sync-generated declaration is editable via the refresh', () => {
    if (!supportsSyncRefresh()) {
      return;
    }
    const importerFile = getRealPath(projectName, 'entry/Importer.ets');
    const importerSource = fs.readFileSync(importerFile, 'utf8');

    // A consumer edit before the sync fills the arktsconfig dependencies parse cache
    // with the current generation (the reference queries share it); without the cache
    // drop on update() the freshly generated entry below would stay unknown to the
    // refresh routing and its edit would take the regular project-file path.
    getLspInstance().modifyFilesMap(importerFile, { newDoc: importerSource });
    expect(consumerDiagnostics().some((message) => message.includes('HarHelper'))).toBe(false);

    // The declgen task generates a brand new declaration the manifest has not recorded
    // yet; the IDE notices and triggers the sync, which rewrites the config in place.
    const extraDeclPath = path.join(declsDir, 'Extra.d.ets');
    fs.writeFileSync(
      extraDeclPath,
      'declare class FreshExport {\n    static val: number;\n}\nexport { FreshExport };\n'
    );
    getLspInstance().update(resolvedModules);

    // The freshly generated declaration is edited in the IDE buffer while the disk copy
    // keeps the generated content: the edit must route through the external refresh so
    // the native import cache holds the buffer content for the new entry.
    const editedDecl = 'declare class RenamedExport {\n    static val: number;\n}\nexport { RenamedExport };\n';
    getLspInstance().modifyFilesMap(extraDeclPath, { newDoc: editedDecl });

    // The consumer imports the renamed export through the new file-level key: neither
    // the symbol nor its module specifier may fail to resolve (a stale import cache
    // would serve the generated disk content and leave the renamed symbol unresolved).
    const updatedSource = importerSource
      .replace(
        "import { HarHelper } from 'syncdecl';",
        "import { HarHelper } from 'syncdecl';\nimport { RenamedExport } from 'syncdecl/Extra';"
      )
      .replace('return HarHelper.val;', 'return HarHelper.val + RenamedExport.val;');
    getLspInstance().modifyFilesMap(importerFile, { newDoc: updatedSource });
    const messages = consumerDiagnostics();
    expect(messages.some((message) => message.includes('RenamedExport') || message.includes('syncdecl/Extra'))).toBe(
      false
    );
  });
});
