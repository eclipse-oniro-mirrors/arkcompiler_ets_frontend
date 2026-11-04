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

const projectName = 'getSubpathImportFix';

describe('getSubpathImportFixTest', () => {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'library', moduleType: 'har', srcPath: 'library' }
  ];
  // The library module maps its module root onto the package name through the
  // 'paths' section; its symbol Foo is only exported by an inner file and deliberately not
  // re-exported by the entry, so the importable specifier keeps the file's path under the
  // module root instead of the bare module alias (which the resolver maps onto the entry
  // file only).
  let lsp: ReturnType<typeof getMultiModuleLsp> | undefined;
  const getLspInstance = (): ReturnType<typeof getMultiModuleLsp> => {
    if (lsp === undefined) {
      lsp = getMultiModuleLsp(projectName, modules, []);
      lsp.initAstCache();
    }
    return lsp;
  };

  test('auto import completion keeps the sub-path of the exporting file', () => {
    const file = getRealPath(projectName, 'entry/Query.ets');
    const source = fs.readFileSync(file, 'utf8');
    const offset = source.lastIndexOf('foo') + 'foo'.length;

    const res = getLspInstance().getCompletionAtPosition(file, offset);
    const entries = (res?.entries ?? []).filter((entry) => entry.name === 'Foo');

    expect(entries.length).toBe(1);
    expect(entries[0].hasAction).toBe(true);
    expect(String(entries[0].data?.importDeclaration ?? '')).toBe('library/utils/Foo');
  });

  test('import fix inserts the sub-path import of the exporting file', () => {
    const file = getRealPath(projectName, 'entry/Page.ets');
    const source = fs.readFileSync(file, 'utf8');
    const offset = source.indexOf('new Foo()') + 'new '.length;

    const res = getLspInstance().getCodeFixesAtPosition(file, offset, offset + 'Foo'.length, [1005]);
    expect(res).toBeDefined();
    const actions = res ?? [];
    // The library entry does not re-export Foo, so the bare module alias would not resolve it.
    expect(
      actions.some((action) => String(action.description.valueOf()) === "Add import {Foo} from 'library'")
    ).toBe(false);
    const importFix = actions.find(
      (action) => String(action.description.valueOf()) === "Add import {Foo} from 'library/utils/Foo'"
    );
    expect(importFix).toBeDefined();
    expect(String(importFix!.description.valueOf())).toBe("Add import {Foo} from 'library/utils/Foo'");
    expect(importFix!.changes.length).toBe(1);
    const textChanges = importFix!.changes[0].textChanges;
    expect(textChanges.length).toBe(1);
    expect(String(textChanges[0].newText)).toBe("import { Foo } from 'library/utils/Foo';\n");
  });

  test('the module entry itself still maps to the bare module alias', () => {
    const file = getRealPath(projectName, 'entry/Query.ets');
    const source = fs.readFileSync(file, 'utf8');
    const offset = source.lastIndexOf('othe') + 'othe'.length;

    const res = getLspInstance().getCompletionAtPosition(file, offset);
    const entries = (res?.entries ?? []).filter((entry) => entry.name === 'Other');

    // The entry-exported Other of the same module keeps the bare alias form.
    expect(entries.length).toBe(1);
    expect(String(entries[0].data?.importDeclaration ?? '')).toBe('library');
  });
});
