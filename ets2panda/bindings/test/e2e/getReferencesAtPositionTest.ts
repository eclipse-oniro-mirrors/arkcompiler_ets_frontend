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

describe('getReferencesAtPositionTest', () => {
  const moduleName: string = 'getReferencesAtPosition';
  const REFERENCES_000 = [{ fileName: 'getReferencesAtPosition1.ets', start: 613, length: 1 }];
  const REFERENCES_001 = [
    { fileName: 'getReferencesAtPosition2.ets', start: 620, length: 1 },
    { fileName: 'getReferencesAtPosition2.ets', start: 635, length: 1 },
    { fileName: 'getReferencesAtPosition2.ets', start: 665, length: 1 },
    { fileName: 'getReferencesAtPosition3.ets', start: 617, length: 1 },
    { fileName: 'getReferencesAtPosition3.ets', start: 667, length: 1 }
  ];
  const REFERENCES_002 = [
    { fileName: 'getReferencesAtPosition4.ets', start: 625, length: 1 },
    { fileName: 'getReferencesAtPosition5.ets', start: 617, length: 1 },
    { fileName: 'getReferencesAtPosition5.ets', start: 655, length: 1 }
  ];
  const REFERENCES_003 = [
    { fileName: 'getReferencesAtPosition6.ets', start: 695, length: 4 },
    { fileName: 'getReferencesAtPosition6.ets', start: 708, length: 4 }
  ];
  function expectReferences(references: any, expected: { fileName: string; start: number; length: number }) {
    references.fileName = path.basename(references.fileName);
    expect(references).toMatchObject(expected);
  }
  const lsp = getLsp(moduleName);
  test('getReferencesAtPosition_000', () => {
    const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition1.ets'), 613);
    expect(res?.length).toBe(1);
    expectReferences(res ? res[0] : undefined, REFERENCES_000[0]);
  });
  test('getReferencesAtPosition_001', () => {
    const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition2.ets'), 635);
    expect(res?.length).toBe(5);
    const length = res ? res.length : 0;
    for (let i = 0; i < length; i++) {
      expectReferences(res ? res[i] : undefined, REFERENCES_001[i]);
    }
  });
  test('getReferencesAtPosition_002', () => {
    const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition4.ets'), 625);
    expect(res?.length).toBe(3);
    const length = res ? res.length : 0;
    for (let i = 0; i < length; i++) {
      expectReferences(res ? res[i] : undefined, REFERENCES_002[i]);
    }
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getReferencesAtPosition_003', () => {
    const res = lsp.getReferencesAtPosition(getRealPath(moduleName, 'getReferencesAtPosition6.ets'), 697);
    expect(res?.length).toBe(2);
    const length = res ? res.length : 0;
    for (let i = 0; i < length; i++) {
      expectReferences(res ? res[i] : undefined, REFERENCES_003[i]);
    }
  });
});
