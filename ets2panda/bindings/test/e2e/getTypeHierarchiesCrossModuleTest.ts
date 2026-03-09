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

describe('getTypeHierarchiesCrossModuleTest', () => {
  const projectName = 'getTypeHierarchiesCrossModule';
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const harBaseFile = getRealPath(projectName, 'har/Base.ets');
  const entryFile = getRealPath(projectName, 'entry/DerivedTypeHierarchy.ets');

  test('getTypeHierarchies_cross_module_superclass', () => {
    const offset = getMarkerOffset(entryFile, '/*derivedTarget*/');
    const res = lsp.getTypeHierarchies(entryFile, offset);

    expect(res).toBeDefined();
    expect(res?.name.valueOf()).toBe('Derived');
    expect(
      res?.superHierarchies.subOrSuper.some(
        (item) => path.resolve(item.fileName.valueOf()) === harBaseFile && item.name.valueOf() === 'Base'
      )
    ).toBe(true);
  });
});
