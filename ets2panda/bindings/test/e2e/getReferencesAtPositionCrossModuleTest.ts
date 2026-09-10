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
  const entryFile1 = getRealPath(projectName, 'entry/EntryReferences1.ets');
  const entryFile2 = getRealPath(projectName, 'entry/EntryReferences2.ets');
  const harFile1 = getRealPath(projectName, 'har/Index.ets');
  const harFile2 = getRealPath(projectName, 'har/Symbols.ets');
  const crossModuleTest = process.env.SKIP_UI_PLUGINS ? test.skip : test;

  crossModuleTest('getReferencesAtPosition_cross_module_class_and_const', () => {
    const lsp = getMultiModuleLsp(projectName, modules, []);
    lsp.initAstCache();
    lsp.modifyFilesMap(entryFile2, { newDoc: fs.readFileSync(entryFile2, 'utf8') });
    lsp.modifyFilesMap(entryFile1, { newDoc: fs.readFileSync(entryFile1, 'utf8') });
    lsp.modifyFilesMap(harFile1, { newDoc: fs.readFileSync(harFile1, 'utf8') });
    lsp.modifyFilesMap(harFile2, { newDoc: fs.readFileSync(harFile2, 'utf8') });

    const classOffset = getMarkerOffset(entryFile1, '/*classTarget*/');
    const classReferences = lsp.getReferencesAtPosition(entryFile1, classOffset);
    expect(classReferences?.length).toBe(8);
    const classReferenceCount = classReferences ? classReferences.length : 0;
    for (let i = 0; i < classReferenceCount; i++) {
      expectReferences(classReferences ? classReferences[i] : undefined, REFERENCES_001[i]);
    }

    const constOffset = getMarkerOffset(entryFile1, '/*constTarget*/');
    const constReferences = lsp.getReferencesAtPosition(entryFile1, constOffset);
    expect(constReferences?.length).toBe(6);
    const constReferenceCount = constReferences ? constReferences.length : 0;
    for (let i = 0; i < constReferenceCount; i++) {
      expectReferences(constReferences ? constReferences[i] : undefined, REFERENCES_002[i]);
    }
  });
});
