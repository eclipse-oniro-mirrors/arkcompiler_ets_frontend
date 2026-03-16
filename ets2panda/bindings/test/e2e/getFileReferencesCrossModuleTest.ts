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
import path from 'path';

import { ModuleDescriptor } from '../../src';
import { getMultiModuleLsp, getRealPath } from '../utils';

function getReferenceText(filePath: string, start: number, length: number): string {
  const source = fs.readFileSync(filePath, 'utf8');
  return source.slice(start, start + length).trim();
}

function getModuleSpecifierLocation(filePath: string, specifier: string) {
  const source = fs.readFileSync(filePath, 'utf8');
  const target = `'${specifier}'`;
  const start = source.indexOf(target);
  if (start === -1) {
    throw new Error(`Specifier ${target} not found in ${filePath}`);
  }
  return {
    fileName: path.resolve(filePath),
    start,
    length: target.length,
    text: target
  };
}

describe('getFileReferencesCrossModuleTest', () => {
  const projectName = 'getFileReferencesCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const harIndexFile = getRealPath(projectName, 'har/Index.ets');
  const harSymbolsFile = getRealPath(projectName, 'har/Symbols.ets');
  const harInternalFile = getRealPath(projectName, 'har/InternalFileReferences.ets');
  const entryFile1 = getRealPath(projectName, 'entry/EntryFileReferences1.ets');
  const entryFile2 = getRealPath(projectName, 'entry/EntryFileReferences2.ets');
  const EXPECT_PACKAGE_REFERENCES = [
    getModuleSpecifierLocation(entryFile1, 'har'),
    getModuleSpecifierLocation(entryFile2, 'har')
  ];
  const EXPECT_INTERNAL_REFERENCES = [
    getModuleSpecifierLocation(harInternalFile, './Symbols')
  ];

  test('getFileReferences_package_entry_is_referenced_by_entry_modules', () => {
    const res = lsp.getFileReferences(harIndexFile);

    expect(res).toBeDefined();
    const refs = (res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName.valueOf()),
      start: ref.start.valueOf(),
      length: ref.length.valueOf(),
      text: getReferenceText(path.resolve(ref.fileName.valueOf()), ref.start.valueOf(), ref.length.valueOf())
    }));

    expect(refs).toHaveLength(EXPECT_PACKAGE_REFERENCES.length);
    expect(refs).toMatchObject(EXPECT_PACKAGE_REFERENCES);
  });

  test('getFileReferences_source_file_is_referenced_inside_har', () => {
    const res = lsp.getFileReferences(harSymbolsFile);

    expect(res).toBeDefined();
    const refs = (res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName.valueOf()),
      start: ref.start.valueOf(),
      length: ref.length.valueOf(),
      text: getReferenceText(path.resolve(ref.fileName.valueOf()), ref.start.valueOf(), ref.length.valueOf())
    }));

    expect(refs).toHaveLength(EXPECT_INTERNAL_REFERENCES.length);
    expect(refs).toMatchObject(EXPECT_INTERNAL_REFERENCES);
  });
});
