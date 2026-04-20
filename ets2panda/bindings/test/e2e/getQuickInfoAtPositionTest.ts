/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { getLsp, getLspWithUi, getRealPath, getSDKPath } from '../utils';

describe('getQuickInfoAtPositionTest', () => {
  const moduleName: string = 'getQuickInfoAtPosition';
  const EXPECT_000 = {
    kind: 'enum member',
    fileName: getRealPath(moduleName, 'getQuickInfoAtPosition1.ets'),
    textSpan: { start: 639, length: 1 },
    displayParts: [
      {
        text: 'MyStrings',
        kind: 'enumName'
      },
      {
        text: '.',
        kind: 'punctuation'
      },
      {
        text: 'A',
        kind: 'enumMember'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: '=',
        kind: 'operator'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: '"',
        kind: 'punctuation'
      },
      {
        text: 'hello',
        kind: 'text'
      },
      {
        text: '"',
        kind: 'punctuation'
      }
    ]
  };
  const EXPECT_001 = {
    kind: 'class',
    kindModifier: '',
    textSpan: {
      start: 628,
      length: 7
    },
    fileName: getRealPath(moduleName, 'getQuickInfoAtPosition2.ets'),
    displayParts: [
      {
        text: 'class',
        kind: 'keyword'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: 'MyClass',
        kind: 'className'
      }
    ]
  };
  const EXPECT_002 = {
    kind: 'get',
    kindModifier: 'public abstract',
    textSpan: {
      start: 674,
      length: 3
    },
    fileName: getRealPath(moduleName, 'getQuickInfoAtPosition3.ets'),
    displayParts: [
      {
        text: 'objI',
        kind: 'interface'
      },
      {
        text: '.',
        kind: 'punctuation'
      },
      {
        text: 'key',
        kind: 'property'
      },
      {
        text: ':',
        kind: 'punctuation'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: 'string',
        kind: 'returnType'
      }
    ]
  };
  const EXPECT_003 = {
    kind: 'method',
    kindModifier: 'public declare',
    textSpan: {
      start: 708,
      length: 4
    },
    fileName: getSDKPath('@ohos.util.Stack.d.ets'),
    displayParts: [
      {
        text: 'Stack',
        kind: 'className'
      },
      {
        text: '.',
        kind: 'punctuation'
      },
      {
        text: 'push',
        kind: 'functionName'
      },
      {
        text: '(',
        kind: 'punctuation'
      },
      {
        text: 'item',
        kind: 'functionParameter'
      },
      {
        text: ':',
        kind: 'punctuation'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: 'T',
        kind: 'typeParameter'
      },
      {
        text: ')',
        kind: 'punctuation'
      },
      {
        text: ':',
        kind: 'punctuation'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: 'T',
        kind: 'returnType'
      }
    ],
    documentation: [
      {
        text: 'Adds an element at the top of this Stack.',
        kind: 'plaintext',
        index: 0
      }
    ],
    tags: [
      {
        name: 'param',
        text: '{ T } item - Target element.',
        index: 0
      },
      {
        name: 'returns',
        text: '{ T } Element added.',
        index: 0
      },
      {
        name: 'throws',
        text: '{ BusinessError } 10200011 - The push method cannot be bound.',
        index: 0
      },
      {
        name: 'syscap',
        text: 'SystemCapability.Utils.Lang',
        index: 0
      },
      {
        name: 'crossplatform',
        index: 0
      },
      {
        name: 'atomicservice',
        index: 0
      },
      {
        name: 'since',
        text: '23',
        index: 0
      }
    ]
  };
  const EXPECT_004 = {
    kind: 'struct',
    kindModifier: '',
    textSpan: {
      start: 699,
      length: 5
    },
    fileName: getRealPath(moduleName, 'getQuickInfoAtPosition5.ets'),
    displayParts: [
      {
        text: 'struct',
        kind: 'keyword'
      },
      {
        text: ' ',
        kind: 'space'
      },
      {
        text: 'Index',
        kind: 'structName'
      }
    ]
  };
  describe('No UI Plugins', () => {
    const lsp = getLsp(moduleName);
    test('getQuickInfoAtPosition_000', () => {
      const res = lsp.getQuickInfoAtPosition(getRealPath(moduleName, 'getQuickInfoAtPosition1.ets'), 639);
      expect(res).toMatchObject(EXPECT_000);
      expect(res?.documentation).toEqual([]);
      expect(res?.tags).toEqual([]);
    });
    test('getQuickInfoAtPosition_001', () => {
      const res = lsp.getQuickInfoAtPosition(getRealPath(moduleName, 'getQuickInfoAtPosition2.ets'), 631);
      expect(res).toMatchObject(EXPECT_001);
      expect(res?.documentation).toEqual([]);
      expect(res?.tags).toEqual([]);
    });
    test('getQuickInfoAtPosition_002', () => {
      const res = lsp.getQuickInfoAtPosition(getRealPath(moduleName, 'getQuickInfoAtPosition3.ets'), 676);
      expect(res).toMatchObject(EXPECT_002);
      expect(res?.documentation).toEqual([]);
      expect(res?.tags).toEqual([]);
    });
  });

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getQuickInfoAtPosition_003', () => {
      const res = getUiLsp().getQuickInfoAtPosition(getRealPath(moduleName, 'getQuickInfoAtPosition4.ets'), 710);
      expect(res).toMatchObject(EXPECT_003);
      expect(res?.documentation).toEqual(EXPECT_003.documentation);
      expect(res?.tags).toEqual(EXPECT_003.tags);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getQuickInfoAtPosition_004', () => {
      const res = getUiLsp().getQuickInfoAtPosition(getRealPath(moduleName, 'getQuickInfoAtPosition5.ets'), 701);
      expect(res).toMatchObject(EXPECT_004);
      expect(res?.documentation).toEqual([]);
      expect(res?.tags).toEqual([]);
    });
  });
});
