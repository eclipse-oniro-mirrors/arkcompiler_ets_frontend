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

function displayText(quickInfo: { displayParts?: Array<{ text: String }> } | undefined): string {
  return quickInfo?.displayParts?.map((part) => part.text.valueOf()).join('') ?? '';
}

describe('getQuickInfoAtPositionCrossModuleTest', () => {
  const projectName = 'getQuickInfoAtPositionCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const entryFile = getRealPath(projectName, 'entry/EntryQuickInfo.ets');

  test('getQuickInfoAtPosition_cross_module_class', () => {
    const offset = getMarkerOffset(entryFile, '/*classTarget*/');
    const res = lsp.getQuickInfoAtPosition(entryFile, offset);

    expect(res).toBeDefined();
    expect(res?.kind).toBe('class');
    expect(path.resolve(res?.fileName.valueOf() ?? '')).toBe(entryFile);
    expect(displayText(res)).toContain('Foo');
  });

  test('getQuickInfoAtPosition_cross_module_const', () => {
    const offset = getMarkerOffset(entryFile, '/*constTarget*/');
    const res = lsp.getQuickInfoAtPosition(entryFile, offset);

    expect(res).toBeDefined();
    expect(path.resolve(res?.fileName.valueOf() ?? '')).toBe(entryFile);
    expect(displayText(res)).toContain('answer');
  });
});
