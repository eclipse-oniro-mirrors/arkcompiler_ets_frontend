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

describe('findRenameLocationsTest', () => {
  const moduleName: string = 'findRenameLocations';
  const EXPECT_000 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 630,
      end: 633,
      line: 15,
      prefixText: 'Foo as '
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 738,
      end: 741,
      line: 23
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 781,
      end: 784,
      line: 24
    }
  ];
  const EXPECT_001 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 625,
      end: 628,
      line: 15
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 1259,
      end: 1262,
      line: 49
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 1267,
      end: 1270,
      line: 50
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 1275,
      end: 1278,
      line: 51
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 625,
      end: 628,
      line: 15
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 694,
      end: 697,
      line: 19
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 702,
      end: 705,
      line: 20
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 711,
      end: 714,
      line: 21
    }
  ];
  const EXPECT_002 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 667,
      end: 672,
      line: 18
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 1239,
      end: 1244,
      line: 47
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 1249,
      end: 1254,
      line: 48
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 618,
      end: 623,
      line: 15
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 673,
      end: 678,
      line: 17
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 683,
      end: 688,
      line: 18
    }
  ];
  const EXPECT_003 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 718,
      end: 722,
      line: 22
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 882,
      end: 886,
      line: 27
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 866,
      end: 870,
      line: 28
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations3.ets'),
      start: 728,
      end: 732,
      line: 18
    }
  ];
  const EXPECT_004 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 618,
      end: 623,
      line: 15,
      prefixText: 'dummy as '
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 673,
      end: 678,
      line: 17
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 683,
      end: 688,
      line: 18
    }
  ];
  const EXPECT_005 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 718,
      end: 722,
      line: 22
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 882,
      end: 886,
      line: 27
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 866,
      end: 870,
      line: 28
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations3.ets'),
      start: 728,
      end: 732,
      line: 18
    }
  ];
  const EXPECT_006 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 718,
      end: 722,
      line: 22
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations1.ets'),
      start: 882,
      end: 886,
      line: 27
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations2.ets'),
      start: 866,
      end: 870,
      line: 28
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations3.ets'),
      start: 728,
      end: 732,
      line: 18
    }
  ];
  const EXPECT_007 = [
    {
      fileName: getRealPath(moduleName, 'findRenameLocations3.ets'),
      start: 625,
      end: 629,
      line: 15
    },
    {
      fileName: getRealPath(moduleName, 'findRenameLocations3.ets'),
      start: 685,
      end: 689,
      line: 17
    }
  ];

  const lsp = getLsp(moduleName);
  test('findRenameLocations_000', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations2.ets'), 632);
    expect(res).toMatchObject(EXPECT_000);
  });
  test('findRenameLocations_001', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations1.ets'), 627);
    expect(res).toMatchObject(EXPECT_001);
  });
  test('findRenameLocations_002', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations1.ets'), 670);
    expect(res).toMatchObject(EXPECT_002);
  });
  test('findRenameLocations_003', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations1.ets'), 721);
    expect(res).toMatchObject(EXPECT_003);
  });
  test('findRenameLocations_004', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations2.ets'), 676);
    expect(res).toMatchObject(EXPECT_004);
  });
  test('findRenameLocations_005', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations2.ets'), 868);
    expect(res).toMatchObject(EXPECT_005);
  });
  test('findRenameLocations_006', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations1.ets'), 720);
    expect(res).toMatchObject(EXPECT_006);
  });
  test('findRenameLocations_007', () => {
    const res = lsp.findRenameLocations(getRealPath(moduleName, 'findRenameLocations3.ets'), 627);
    expect(res).toMatchObject(EXPECT_007);
  });
});
