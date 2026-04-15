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

function expectReferences(references: any, expected: { fileName: string; start: number; length: number }) {
  references.fileName = path.basename(references.fileName);
  expect(references).toMatchObject(expected);
}

describe('getReferencesAtPositionCrossModuleTest', () => {
  const projectName = 'getReferencesAtPositionCrossModule';
  const REFERENCES_001 = [
    { fileName: 'EntryReferences1.ets', start: 632, length: 3 },
    { fileName: 'EntryReferences1.ets', start: 699, length: 3 },
    { fileName: 'EntryReferences2.ets', start: 632, length: 3 },
    { fileName: 'EntryReferences2.ets', start: 684, length: 3 },
    { fileName: 'EntryReferences2.ets', start: 694, length: 3 },
    { fileName: 'Index.ets', start: 632, length: 3 },
    { fileName: 'Symbols.ets', start: 716, length: 3 },
    { fileName: 'Symbols.ets', start: 735, length: 3 }
  ];
  const REFERENCES_002 = [
    { fileName: 'EntryReferences1.ets', start: 637, length: 6 },
    { fileName: 'EntryReferences1.ets', start: 735, length: 6 },
    { fileName: 'EntryReferences2.ets', start: 637, length: 6 },
    { fileName: 'EntryReferences2.ets', start: 721, length: 6 },
    { fileName: 'Index.ets', start: 637, length: 6 },
    { fileName: 'Symbols.ets', start: 814, length: 6 }
  ];
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const entryFile1 = getRealPath(projectName, 'entry/EntryReferences1.ets');
  const entryFile2 = getRealPath(projectName, 'entry/EntryReferences2.ets');
  const harFile1 = getRealPath(projectName, 'har/Index.ets');
  const harFile2 = getRealPath(projectName, 'har/Symbols.ets');
  lsp.modifyFilesMap(harFile1, { newDoc: fs.readFileSync(harFile1, 'utf8') });
  lsp.modifyFilesMap(harFile2, { newDoc: fs.readFileSync(harFile2, 'utf8') });
  lsp.modifyFilesMap(entryFile2, { newDoc: fs.readFileSync(entryFile2, 'utf8') });
  lsp.modifyFilesMap(entryFile1, { newDoc: fs.readFileSync(entryFile1, 'utf8') });

  test('getReferencesAtPosition_cross_module_class', () => {
    const offset = getMarkerOffset(entryFile1, '/*classTarget*/');
    const res = lsp.getReferencesAtPosition(entryFile1, offset);
    expect(res?.length).toBe(8);
    const length = res ? res.length : 0;
    for (let i = 0; i < length; i++) {
      expectReferences(res ? res[i] : undefined, REFERENCES_001[i]);
    }
  });

  test('getReferencesAtPosition_cross_module_const', () => {
    const offset = getMarkerOffset(entryFile1, '/*constTarget*/');
    const res = lsp.getReferencesAtPosition(entryFile1, offset);
    expect(res?.length).toBe(6);
    const length = res ? res.length : 0;
    for (let i = 0; i < length; i++) {
      expectReferences(res ? res[i] : undefined, REFERENCES_002[i]);
    }
  });
});
