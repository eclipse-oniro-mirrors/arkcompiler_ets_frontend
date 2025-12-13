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

describe('getSignatureHelpItemsTest', () => {
  const moduleName: string = 'getSignatureHelpItems';
  const EXPECT_000 = {
    items: [
      {
        prefixDisplayParts: [
          {
            text: 'add',
            kind: 'functionName'
          },
          {
            text: '(',
            kind: 'punctuation'
          }
        ],
        suffixDisplayParts: [
          {
            text: ')',
            kind: 'punctuation'
          },
          {
            text: ' ',
            kind: 'space'
          },
          {
            text: '=>',
            kind: 'punctuation'
          },
          {
            text: ' ',
            kind: 'space'
          },
          {
            text: 'number',
            kind: 'keyword'
          }
        ],
        separatorDisplayParts: [
          {
            text: ',',
            kind: 'punctuation'
          },
          {
            text: ' ',
            kind: 'space'
          }
        ],
        parameters: [
          {
            name: 'a',
            documentation: [],
            displayParts: [
              {
                text: 'a',
                kind: 'parameterNmae'
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
                text: 'number',
                kind: 'keyword'
              }
            ]
          },
          {
            name: 'b',
            documentation: [],
            displayParts: [
              {
                text: 'b',
                kind: 'parameterNmae'
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
                text: 'number',
                kind: 'keyword'
              }
            ]
          }
        ],
        documentation: []
      }
    ],
    applicableSpan: {
      start: 678,
      length: 0
    },
    selectedItemIndex: 0,
    argumentIndex: 0,
    argumentCount: 2
  };
  const lsp = getLsp(moduleName);
  test('getSignatureHelpItems_000', () => {
    const res = lsp.getSignatureHelpItems(getRealPath(moduleName, 'getSignatureHelpItems1.ets'), 678);
    expect(res).toMatchObject(EXPECT_000);
  });
});
