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

const projectName = 'getCompletionsAutoImportCrossModule';
const harRoot = path.resolve('test/testcases', projectName, 'har');
const buildDeclDir = path.join(harRoot, 'build', 'default', 'intermediates', 'declgen', 'default');

// The build system emits the dynamic module decl file info with absolute paths; it is
// materialized here before the LSP is constructed, mirroring a built project. The module
// entry and two sibling files A/B each carry their own declaration: A and B both export a
// same-named class Foo while the entry Index re-exports only A's (re-exporting both would
// be a duplicate export error).
function writeDeclFileInfo(): void {
  const declsDir = path.join(buildDeclDir, 'decls');
  fs.mkdirSync(declsDir, { recursive: true });
  fs.writeFileSync(
    path.join(declsDir, 'Index.d.ets'),
    'declare class HarHelper {\n    static val: number;\n}\nexport { HarHelper };\nexport { Foo } from \'./A\';\n'
  );
  fs.writeFileSync(
    path.join(declsDir, 'A.d.ets'),
    'declare class Foo {\n    fromA(): number;\n}\nexport { Foo };\n'
  );
  fs.writeFileSync(
    path.join(declsDir, 'B.d.ets'),
    'declare class Foo {\n    fromB(): number;\n}\nexport { Foo };\n'
  );
  fs.writeFileSync(
    path.join(buildDeclDir, 'decl-fileInfo.json'),
    JSON.stringify(
      {
        packageName: 'har',
        files: {
          Index: {
            declPath: path.join(declsDir, 'Index.d.ets'),
            filePath: path.join(harRoot, 'Index.ets'),
            ohmUrl: ''
          },
          A: { declPath: path.join(declsDir, 'A.d.ets'), filePath: path.join(harRoot, 'A.ets'), ohmUrl: '' },
          B: { declPath: path.join(declsDir, 'B.d.ets'), filePath: path.join(harRoot, 'B.ets'), ohmUrl: '' }
        }
      },
      null,
      2
    )
  );
}

writeDeclFileInfo();

describe('getCompletionsAutoImportCrossModuleTest', () => {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  // Every case builds the AST cache through getLspInstance (initAstCache), which
  // resolves the fixture's dynamic SDK declarations; environments whose ets/dynamic
  // fixture cannot be parsed run with SKIP_UI_PLUGINS and skip them.
  let lsp: ReturnType<typeof getMultiModuleLsp> | undefined;
  const getLspInstance = (): ReturnType<typeof getMultiModuleLsp> => {
    if (lsp === undefined) {
      lsp = getMultiModuleLsp(projectName, modules, []);
      lsp.initAstCache();
    }
    return lsp;
  };

  afterAll(() => {
    fs.rmSync(path.join(harRoot, 'build'), { recursive: true, force: true });
  });

  function queryCompletions(token: string) {
    const queryFile = getRealPath(projectName, 'entry/Query.ets');
    const source = fs.readFileSync(queryFile, 'utf8');
    const offset = source.lastIndexOf(token) + token.length;
    return getLspInstance().getCompletionAtPosition(queryFile, offset);
  }

  function queryCompletionsInFile(fileName: string, token: string) {
    const queryFile = getRealPath(projectName, fileName);
    const source = fs.readFileSync(queryFile, 'utf8');
    const offset = source.indexOf(token) + token.length;
    return getLspInstance().getCompletionAtPosition(queryFile, offset);
  }

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('cross module auto import entry is unique per module specifier', () => {
    // The HarHelper symbol is indexed from both the har source file and its declaration;
    // both resolve to the same module specifier 'har' and must yield a single entry.
    const res = queryCompletions('harH');
    const entries = (res?.entries ?? []).filter((entry) => entry.name === 'HarHelper');

    expect(entries.length).toBe(1);
    expect(entries[0].hasAction).toBe(true);
    expect(String(entries[0].data?.importDeclaration ?? '')).toBe('har');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('sdk auto import entries keep one entry per import form', () => {
    // The hilog SDK symbol carries four intentional import forms (static canonical,
    // dynamic canonical and the kit re-export of each root); each form is emitted once.
    // The array comparison keeps the failure output listing every actually received
    // import declaration when the forms drift.
    const res = queryCompletions('hilo');
    const entries = (res?.entries ?? []).filter((entry) => entry.name === 'hilog');

    const importDeclarations = entries.map((entry) => String(entry.data?.importDeclaration ?? ''));
    expect([...importDeclarations].sort()).toEqual(
      ['@kit.PerformanceAnalysisKit', '@ohos.hilog', 'dynamic@kit.PerformanceAnalysisKit', 'dynamic@ohos.hilog']
    );
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('dynamic module declaration joins the module symbol key', () => {
    // The declgen declaration of the dynamic 'har' module (Index.d.ets) registers under
    // the importers' 'module:har:<symbol>' key - the 'dependencies' entry 'har/Index'
    // carries no 'dynamic@' prefix, its first segment is the importable name - so both
    // the reference list and the rename locations cover the declaration site.
    const file = getRealPath(projectName, 'entry/Importer.ets');
    const source = fs.readFileSync(file, 'utf8');
    const offset = source.indexOf('HarHelper.val') + 2;

    const refs = getLspInstance().getReferencesAtPosition(file, offset);
    expect((refs ?? []).map((r) => path.basename(r.fileName.valueOf())).sort()).toEqual([
      'Importer.ets', 'Importer.ets', 'Index.d.ets'
    ]);

    const locs = getLspInstance().findRenameLocations(file, offset);
    const declLocations = (locs ?? []).filter((l) => path.basename(l.fileName.valueOf()) === 'Index.d.ets');
    expect(declLocations.length).toBe(1);
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('import string literal suggests the cross module alias', () => {
    // Typing 'ha' inside import { x } from 'ha' suggests the 'har' module specifier.
    const res = queryCompletionsInFile('entry/PartialImport.ets', "'ha'");
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());

    expect(names).toContain('har');
    const harEntry = (res?.entries ?? []).find((entry) => entry.name === 'har');
    expect(harEntry?.insertText).toBe('har');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('import string literal suggests both sdk import forms', () => {
    // Typing '@ohos.hi' inside the import string suggests both '@ohos.hilog' and
    // 'dynamic@ohos.hilog'.
    const res = queryCompletionsInFile('entry/PartialImport.ets', "'@ohos.hi'");
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());

    expect(names).toContain('@ohos.hilog');
    expect(names).toContain('dynamic@ohos.hilog');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('import string literal hides sdk internal category modules', () => {
    // The SDK api category modules (e.g. 'arkui.SymbolGlyphModifier') are imported
    // only by the SDK's own '@'-prefixed declarations and must not be suggested.
    const res = queryCompletionsInFile('entry/PartialImport.ets', "'arkui.SymbolGlyphModifier'");
    const names = (res?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(names.some((name) => name.startsWith('arkui.'))).toBe(false);

    // The standard library and the project module stay visible.
    const stdRes = queryCompletionsInFile('entry/PartialImport.ets', "'std");
    const stdNames = (stdRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(stdNames).toContain('std');
    const harRes = queryCompletionsInFile('entry/PartialImport.ets', "'ha'");
    const harNames = (harRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(harNames).toContain('har');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('import string literal keeps suggesting after a trailing @', () => {
    // Typing 'dynamic@' or '@' inside the import string must keep suggesting the
    // module specifiers: a trailing '@' used to be misrouted to the annotation
    // completions by the text-only annotation-beginning check.
    const dynRes = queryCompletionsInFile('entry/PartialImport.ets', "'dynamic@'");
    const dynNames = (dynRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(dynNames).toContain('dynamic@ohos.hilog');

    const atRes = queryCompletionsInFile('entry/PartialImport.ets', "'@'");
    const atNames = (atRes?.entries ?? []).map((entry) => entry.name.valueOf());
    expect(atNames).toContain('@ohos.hilog');
    expect(atNames).toContain('dynamic@ohos.hilog');
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('dynamic sibling same-name symbols keep one entry per file', () => {
    // Two sibling files of the dynamic 'har' module each export a same-named class Foo and
    // the module entry Index re-exports only A's (re-exporting both would be a duplicate
    // export error). The auto import entries keep the file-level module specifiers - one
    // entry per sibling plus the entry re-export - instead of merging them all under the
    // package name.
    const res = queryCompletions('foo');
    const entries = (res?.entries ?? []).filter((entry) => entry.name === 'Foo');

    const importDeclarations = entries.map((entry) => String(entry.data?.importDeclaration ?? ''));
    expect([...importDeclarations].sort()).toEqual(['har', 'har/A', 'har/B']);
  });

  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('dynamic sibling sub-path import stays separate from the other file', () => {
    // The sub-path import of A's Foo binds to A's own symbol: its references cover A's
    // declaration and the importer, never the same-named Foo independently declared in B.
    const file = getRealPath(projectName, 'entry/SubpathImport.ets');
    const source = fs.readFileSync(file, 'utf8');
    const offset = source.indexOf('new Foo()') + 'new '.length;

    const refs = getLspInstance().getReferencesAtPosition(file, offset);
    const refFiles = (refs ?? []).map((r) => String(r.fileName));
    expect(refFiles.some((name) => name.includes('A.ets')) || refFiles.some((name) => name.includes('A.d.ets'))).toBe(
      true
    );
    expect(refFiles.some((name) => path.basename(name) === 'B.ets')).toBe(false);
    expect(refFiles.some((name) => path.basename(name) === 'B.d.ets')).toBe(false);

    const locs = getLspInstance().findRenameLocations(file, offset);
    const locFiles = (locs ?? []).map((l) => String(l.fileName));
    expect(locFiles.some((name) => path.basename(name) === 'B.ets')).toBe(false);
    expect(locFiles.some((name) => path.basename(name) === 'B.d.ets')).toBe(false);
  });
});
