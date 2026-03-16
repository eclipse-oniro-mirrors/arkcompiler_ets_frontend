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

function getMarkerOffset(filePath: string, marker: string): number {
  const source = fs.readFileSync(filePath, 'utf8');
  const markerIndex = source.indexOf(marker);
  if (markerIndex === -1) {
    throw new Error(`Marker ${marker} not found in ${filePath}`);
  }
  return markerIndex + marker.length;
}

function getReferenceText(filePath: string, start: number, length: number): string {
  const source = fs.readFileSync(filePath, 'utf8');
  return source.slice(start, start + length).trim();
}

function hasReference(refs: Array<{ fileName: string; text: string }>, filePath: string, text: string): boolean {
  const targetFile = path.resolve(filePath);
  return refs.some((ref) => ref.fileName === targetFile && ref.text === text);
}

function hasHarReference(refs: Array<{ fileName: string; text: string }>, harDir: string, text: string): boolean {
  const normalizedHarDir = `${path.resolve(harDir)}${path.sep}`;
  return refs.some((ref) => ref.fileName.startsWith(normalizedHarDir) && ref.text === text);
}

describe('getReferencesAtPositionCrossModuleTest', () => {
  const projectName = 'getReferencesAtPositionCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const entryFile1 = getRealPath(projectName, 'entry/EntryReferences1.ets');
  const entryFile2 = getRealPath(projectName, 'entry/EntryReferences2.ets');
  const harDir = getRealPath(projectName, 'har');

  test('getReferencesAtPosition_cross_module_class', () => {
    const offset = getMarkerOffset(entryFile1, '/*classTarget*/');
    const res = lsp.getReferencesAtPosition(entryFile1, offset);

    expect(res).toBeDefined();
    const refs = (res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName.valueOf()),
      text: getReferenceText(path.resolve(ref.fileName.valueOf()), ref.start.valueOf(), ref.length.valueOf())
    }));

    expect(hasHarReference(refs, harDir, 'Foo')).toBe(true);
    expect(hasReference(refs, entryFile1, 'Foo')).toBe(true);
    expect(hasReference(refs, entryFile2, 'Foo')).toBe(true);
  });

  test('getReferencesAtPosition_cross_module_const', () => {
    const offset = getMarkerOffset(entryFile1, '/*constTarget*/');
    const res = lsp.getReferencesAtPosition(entryFile1, offset);

    expect(res).toBeDefined();
    const refs = (res ?? []).map((ref) => ({
      fileName: path.resolve(ref.fileName.valueOf()),
      text: getReferenceText(path.resolve(ref.fileName.valueOf()), ref.start.valueOf(), ref.length.valueOf())
    }));

    expect(hasHarReference(refs, harDir, 'answer')).toBe(true);
    expect(hasReference(refs, entryFile1, 'answer')).toBe(true);
    expect(hasReference(refs, entryFile2, 'answer')).toBe(true);
  });
});
