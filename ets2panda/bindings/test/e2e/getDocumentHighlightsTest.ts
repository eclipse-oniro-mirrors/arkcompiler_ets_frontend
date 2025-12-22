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

describe('getDocumentHighlightsTest', () => {
  const moduleName: string = 'getDocumentHighlights';
  const EXPECT_000 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights1.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights1.ets'),
            textSpan: {
              start: 613,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights1.ets'),
            textSpan: {
              start: 634,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights1.ets'),
            textSpan: {
              start: 661,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_001 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights2.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights2.ets'),
            textSpan: {
              start: 628,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights2.ets'),
            textSpan: {
              start: 655,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights2.ets'),
            textSpan: {
              start: 716,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_002 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights3.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights3.ets'),
            textSpan: {
              start: 615,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights3.ets'),
            textSpan: {
              start: 660,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights3.ets'),
            textSpan: {
              start: 718,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_003 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights4.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights4.ets'),
            textSpan: {
              start: 625,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights4.ets'),
            textSpan: {
              start: 672,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights4.ets'),
            textSpan: {
              start: 741,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights4.ets'),
            textSpan: {
              start: 752,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_004 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights5.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights5.ets'),
            textSpan: {
              start: 618,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights5.ets'),
            textSpan: {
              start: 696,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights5.ets'),
            textSpan: {
              start: 740,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_005 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights6.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights6.ets'),
            textSpan: {
              start: 615,
              length: 8
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights6.ets'),
            textSpan: {
              start: 653,
              length: 8
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_006 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights7.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights7.ets'),
            textSpan: {
              start: 618,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights7.ets'),
            textSpan: {
              start: 732,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights7.ets'),
            textSpan: {
              start: 745,
              length: 3
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_007 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights8.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights8.ets'),
            textSpan: {
              start: 628,
              length: 5
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 3
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights8.ets'),
            textSpan: {
              start: 674,
              length: 5
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const EXPECT_008 = {
    documentHighlights: [
      {
        fileName: getRealPath(moduleName, 'getDocumentHighlights9.ets'),
        highlightSpans: [
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights9.ets'),
            textSpan: {
              start: 618,
              length: 4
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          },
          {
            fileName: getRealPath(moduleName, 'getDocumentHighlights9.ets'),
            textSpan: {
              start: 655,
              length: 4
            },
            contextSpan: {
              start: 0,
              length: 0
            },
            kind: 2
          }
        ]
      }
    ]
  };
  const lsp = getLsp(moduleName);
  test('getDocumentHighlights_000', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights1.ets'), 614);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_000);
  });
  test('getDocumentHighlights_001', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights2.ets'), 717);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_001);
  });
  test('getDocumentHighlights_002', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights3.ets'), 616);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_002);
  });
  test('getDocumentHighlights_003', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights4.ets'), 626);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_003);
  });
  test('getDocumentHighlights_004', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights5.ets'), 619);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_004);
  });
  test('getDocumentHighlights_005', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights6.ets'), 657);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_005);
  });
  test('getDocumentHighlights_006', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights7.ets'), 733);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_006);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getDocumentHighlights_007', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights8.ets'), 677);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_007);
  });
  (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getDocumentHighlights_008', () => {
    const res = lsp.getDocumentHighlights(getRealPath(moduleName, 'getDocumentHighlights9.ets'), 620);
    expect(res?.documentHighlights.length).toBe(1);
    expect(res).toMatchObject(EXPECT_008);
  });
});
