/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { getLsp, getRealPath } from '../utils';
import path from 'path';

describe('getDefinitionAtPositionTest', () => {
  const moduleName: string = 'getDefinitionAtPosition';
  const lsp = getLsp(moduleName);
  test('getDefinitionAtPosition_000', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition2.ets'), 669);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition1.ets');
    expect(res?.start).toBe(639);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_001', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition3.ets'), 676);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition3.ets');
    expect(res?.start).toBe(632);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_002', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition5.ets'), 678);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition4.ets');
    expect(res?.start).toBe(639);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_003', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition7.ets'), 697);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition6.ets');
    expect(res?.start).toBe(640);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionAtPosition_004', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition9.ets'), 680);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition8.ets');
    expect(res?.start).toBe(639);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_005', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition11.ets'), 689);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition10.ets');
    expect(res?.start).toBe(640);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionAtPosition_006', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition13.ets'), 678);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition12.ets');
    expect(res?.start).toBe(634);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_007', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition15.ets'), 631);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition14.ets');
    expect(res?.start).toBe(640);
    expect(res?.length).toBe(1);
  });
  test('getDefinitionAtPosition_008', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition17.ets'), 691);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition16.ets');
    expect(res?.start).toBe(636);
    expect(res?.length).toBe(3);
  });
  test('getDefinitionAtPosition_009', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition19.ets'), 648);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('taskpool.ets');
    expect(res?.length).toBe(4);
  });
  test('getDefinitionAtPosition_010', () => {
    const res = lsp.getDefinitionAtPosition(getRealPath(moduleName, 'getDefinitionAtPosition2.ets'), 651);
    expect(res ? path.basename(res.fileName.valueOf()) : undefined).toBe('getDefinitionAtPosition1.ets');
    expect(res?.start).toBe(0);
    expect(res?.length).toBe(0);
  });
});
