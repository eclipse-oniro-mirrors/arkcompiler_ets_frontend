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

import { LspCompletionInfo } from '../../src';
import { getLsp, getRealPath } from '../utils';

describe('getCompletionAtPositionTest', () => {
  const moduleName: string = 'getCompletionAtPosition';
  const EXPECT_000 = [
    {
      name: 'num2',
      sortText: '15',
      insertText: 'num2()',
      kind: 3,
      data: null
    },
    {
      name: 'num1',
      sortText: '15',
      insertText: 'num1()',
      kind: 3,
      data: null
    }
  ];
  const EXPECT_001 = [
    {
      name: 'axx',
      sortText: '15',
      insertText: 'axx()',
      kind: 3,
      data: null
    },
    {
      name: 'aaa',
      sortText: '15',
      insertText: 'aaa',
      kind: 6,
      data: null
    },
    {
      name: 'abb',
      sortText: '15',
      insertText: 'abb',
      kind: 21,
      data: null
    }
  ];
  const EXPECT_002 = [
    {
      name: 'baa',
      sortText: '15',
      insertText: 'baa',
      kind: 6,
      data: null
    },
    {
      name: 'bbb',
      sortText: '15',
      insertText: 'bbb',
      kind: 6,
      data: null
    },
    {
      name: 'bxx',
      sortText: '15',
      insertText: 'bxx()',
      kind: 3,
      data: null
    },
    {
      name: 'bcc',
      sortText: '15',
      insertText: 'bcc',
      kind: 6,
      data: null
    }
  ];
  const EXPECT_003 = [
    {
      name: 'bxx',
      sortText: '15',
      insertText: 'bxx()',
      kind: 3,
      data: null
    },
    {
      name: 'baa',
      sortText: '15',
      insertText: 'baa',
      kind: 6,
      data: null
    },
    {
      name: 'bbb',
      sortText: '15',
      insertText: 'bbb',
      kind: 6,
      data: null
    }
  ];
  const EXPECT_004 = [
    {
      name: 'myProp',
      sortText: '14',
      insertText: 'myProp',
      kind: 10,
      data: null
    }
  ];
  const EXPECT_005 = [
    {
      name: 'classInSpace',
      sortText: '13',
      insertText: 'classInSpace',
      kind: 7,
      data: null
    }
  ];
  const EXPECT_006 = [
    {
      name: 'Red',
      sortText: '13',
      insertText: 'Red',
      kind: 20,
      data: null
    }
  ];
  const EXPECT_007: any[] = [];
  const EXPECT_008 = [
    {
      name: 'number',
      sortText: '15',
      insertText: 'number',
      kind: 14,
      data: null
    }
  ];
  const EXPECT_009 = [
    {
      name: 'classInSpace',
      sortText: '13',
      insertText: 'classInSpace',
      kind: 7,
      data: null
    }
  ];
  const EXPECT_010 = [
    {
      name: 'Red',
      sortText: '13',
      insertText: 'Red',
      kind: 20,
      data: null
    },
    {
      name: 'Blue',
      sortText: '13',
      insertText: 'Blue',
      kind: 20,
      data: null
    }
  ];
  const EXPECT_011 = [
    {
      name: 'myProp',
      sortText: '14',
      insertText: 'myProp',
      kind: 10,
      data: null
    },
    {
      name: 'prop',
      sortText: '14',
      insertText: 'prop',
      kind: 10,
      data: null
    }
  ];
  const EXPECT_012 = [
    {
      name: 'key',
      sortText: '17',
      insertText: 'key',
      kind: 2,
      data: null
    }
  ];
  const EXPECT_013 = [
    {
      name: 'key',
      sortText: '17',
      insertText: 'key',
      kind: 2,
      data: null
    }
  ];
  const EXPECT_014 = [
    {
      name: 'isEmpty',
      sortText: '17',
      insertText: 'isEmpty()',
      kind: 2,
      data: null
    },
    {
      name: 'peek',
      sortText: '17',
      insertText: 'peek()',
      kind: 2,
      data: null
    },
    {
      name: 'pop',
      sortText: '17',
      insertText: 'pop()',
      kind: 2,
      data: null
    },
    {
      name: 'push',
      sortText: '17',
      insertText: 'push()',
      kind: 2,
      data: null
    }
  ];
  const EXPECT_015 = [
    {
      name: 'Entry',
      sortText: '15',
      insertText: 'Entry',
      kind: 27,
      data: null
    },
    {
      name: 'Entry2',
      sortText: '15',
      insertText: 'Entry2',
      kind: 27,
      data: null
    }
  ];
  const EXPECT_016 = [
    {
      name: 'Entry',
      sortText: '15',
      insertText: 'Entry',
      kind: 27,
      data: null
    },
    {
      name: 'TestAnnotation',
      sortText: '15',
      insertText: 'TestAnnotation',
      kind: 27,
      data: null
    },
    {
      name: 'Entry2',
      sortText: '15',
      insertText: 'Entry2',
      kind: 27,
      data: null
    }
  ];
  const EXPECT_017 = [
    {
      name: 'name',
      sortText: '14',
      insertText: 'name',
      kind: 10,
      data: null
    },
    {
      name: 'age',
      sortText: '14',
      insertText: 'age',
      kind: 10,
      data: null
    },
    {
      name: 'introduce',
      sortText: '17',
      insertText: 'introduce()',
      kind: 2,
      data: null
    }
  ];
  function toMatchObjectUnordered(realValue: LspCompletionInfo | undefined, expect: any, sliceSize: number) {
    for (let i = 0; i <= sliceSize; i++) {
      if (
        realValue?.entries[i].name === expect.name &&
        realValue?.entries[i].sortText === expect.sortText &&
        realValue?.entries[i].insertText === expect.insertText &&
        realValue?.entries[i].kind === expect.kind
      ) {
        return true;
      }
    }
    return false;
  }
  const lsp = getLsp(moduleName);
  test('getCompletionAtPosition_000', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition1.ets'), 705);
    EXPECT_000.forEach((item) => {
      expect(toMatchObjectUnordered(res, item, 2)).toBe(true);
    });
  });
  test('getCompletionAtPosition_001', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition2.ets'), 735);
    EXPECT_001.forEach((item) => {
      expect(toMatchObjectUnordered(res, item, 3)).toBe(true);
    });
  });
  test('getCompletionAtPosition_002', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition3.ets'), 789);
    EXPECT_002.forEach((item) => {
      expect(toMatchObjectUnordered(res, item, 4)).toBe(true);
    });
  });
  test('getCompletionAtPosition_003', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition4.ets'), 767);
    EXPECT_003.forEach((item) => {
      expect(toMatchObjectUnordered(res, item, 3)).toBe(true);
    });
  });
  test('getCompletionAtPosition_004', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition5.ets'), 728);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_004);
  });
  test('getCompletionAtPosition_005', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition6.ets'), 718);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_005);
  });
  test('getCompletionAtPosition_006', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition7.ets'), 683);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_006);
  });
  test('getCompletionAtPosition_007', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition8.ets'), 614);
    expect(res).toBeDefined();
  });
  test('getCompletionAtPosition_008', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition9.ets'), 619);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_008);
  });
  test('getCompletionAtPosition_009', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition10.ets'), 712);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_009);
  });
  test('getCompletionAtPosition_010', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition11.ets'), 682);
    expect(res?.entries.slice(0, 2)).toMatchObject(EXPECT_010);
  });
  test('getCompletionAtPosition_011', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition12.ets'), 720);
    expect(res?.entries.slice(0, 2)).toMatchObject(EXPECT_011);
  });
  test('getCompletionAtPosition_012', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition13.ets'), 658);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_012);
  });
  test('getCompletionAtPosition_013', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition14.ets'), 659);
    expect(res?.entries.slice(0, 1)).toMatchObject(EXPECT_013);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCompletionAtPosition_014', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition15.ets'), 722);
    expect(res?.entries.slice(0, 4)).toMatchObject(EXPECT_014);
  });
  test('getCompletionAtPosition_015', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition17.ets'), 764);
    expect(res?.entries.slice(0, 2)).toMatchObject(EXPECT_015);
  });
  test('getCompletionAtPosition_016', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition17.ets'), 782);
    expect(res?.entries.slice(0, 3)).toMatchObject(EXPECT_016);
  });
  test('getCompletionAtPosition_017', () => {
    const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition18.ets'), 868);
    expect(res?.entries.slice(0, 3)).toMatchObject(EXPECT_017);
  });
});
