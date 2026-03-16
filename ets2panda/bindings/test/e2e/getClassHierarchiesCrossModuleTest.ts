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

describe('getClassHierarchiesCrossModuleTest', () => {
  const projectName = 'getClassHierarchiesCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const harBaseFile = getRealPath(projectName, 'har/Base.ets');
  const entryFile = getRealPath(projectName, 'entry/DerivedClassHierarchy.ets');

  test('getClassHierarchies_cross_module_overriding', () => {
    const offset = getMarkerOffset(harBaseFile, '/*baseMethodTarget*/');
    const res = lsp.getClassHierarchies(harBaseFile, offset);

    expect(res).toBeDefined();
    expect(res?.classHierarchies.length).toBeGreaterThan(0);
  });

  test('getClassHierarchies_cross_module_overridden', () => {
    const offset = getMarkerOffset(entryFile, '/*derivedMethodTarget*/');
    const res = lsp.getClassHierarchies(entryFile, offset);

    expect(res).toBeDefined();
    expect(res?.classHierarchies.length).toBeGreaterThan(0);
  });
});
