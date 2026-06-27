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

import { lspData } from 'src/common/private';
import fs from 'fs';
import path from 'path';
import { LspCompletionInfo } from '../../src';
import { getLsp, getLspWithUi, getRealPath } from '../utils';

describe('getCompletionAtPositionTest', () => {
  const moduleName: string = 'getCompletionAtPosition';
  const EXPECT_000 = [
    {
      name: 'num2(): Int',
      sortText: '15',
      insertText: 'num2()',
      kind: 3,
      data: null
    },
    {
      name: 'num1(): Int',
      sortText: '15',
      insertText: 'num1()',
      kind: 3,
      data: null
    }
  ];
  const EXPECT_001 = [
    {
      name: 'axx(): Int',
      sortText: '15',
      insertText: 'axx()',
      kind: 3,
      data: null
    },
    {
      name: 'aaa: Int',
      sortText: '15',
      insertText: 'aaa',
      kind: 6,
      data: null
    },
    {
      name: 'abb: Int',
      sortText: '15',
      insertText: 'abb',
      kind: 21,
      data: null
    }
  ];
  const EXPECT_002 = [
    {
      name: 'baa: Int',
      sortText: '15',
      insertText: 'baa',
      kind: 6,
      data: null
    },
    {
      name: 'bbb: Int',
      sortText: '15',
      insertText: 'bbb',
      kind: 6,
      data: null
    },
    {
      name: 'bxx(): Int',
      sortText: '15',
      insertText: 'bxx()',
      kind: 3,
      data: null
    },
    {
      name: 'bcc: Int',
      sortText: '15',
      insertText: 'bcc',
      kind: 6,
      data: null
    }
  ];
  const EXPECT_003 = [
    {
      name: 'bxx(): Int',
      sortText: '15',
      insertText: 'bxx()',
      kind: 3,
      data: null
    },
    {
      name: 'baa: Int',
      sortText: '15',
      insertText: 'baa',
      kind: 6,
      data: null
    },
    {
      name: 'bbb: Int',
      sortText: '15',
      insertText: 'bbb',
      kind: 6,
      data: null
    }
  ];
  const EXPECT_004 = [
    {
      name: 'myProp: number',
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
      name: 'myProp: number',
      sortText: '14',
      insertText: 'myProp',
      kind: 10,
      data: null
    },
    {
      name: 'prop: number',
      sortText: '14',
      insertText: 'prop',
      kind: 10,
      data: null
    }
  ];
  const EXPECT_012 = [
    {
      name: 'key: string',
      sortText: '17',
      insertText: 'key',
      kind: 10,
      data: null
    }
  ];
  const EXPECT_013 = [
    {
      name: 'key: string',
      sortText: '17',
      insertText: 'key',
      kind: 10,
      data: null
    }
  ];
  const EXPECT_014 = [
    {
      name: 'isEmpty(): boolean',
      sortText: '17',
      insertText: 'isEmpty()',
      kind: 2,
      data: null
    },
    {
      name: 'peek(): T',
      sortText: '17',
      insertText: 'peek()',
      kind: 2,
      data: null
    },
    {
      name: 'pop(): T',
      sortText: '17',
      insertText: 'pop()',
      kind: 2,
      data: null
    },
    {
      name: 'push(item: T): T',
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
      name: 'name: string',
      sortText: '14',
      insertText: 'name',
      kind: 10,
      data: null
    },
    {
      name: 'age: number',
      sortText: '14',
      insertText: 'age',
      kind: 10,
      data: null
    },
    {
      name: 'introduce(name: string, age: number): void',
      sortText: '17',
      insertText: 'introduce()',
      kind: 2,
      data: null
    }
  ];
  const EXPECT_018 = [
    {
      name: 'ability',
      sortText: '16',
      insertText: 'ability',
      kind: 9,
      data: {
        namedExport: 'ability',
        importDeclaration: '@ohos.ability.ability.d.ets'
      },
      hasAction: true
    },
    {
      name: 'abilityManager',
      sortText: '16',
      insertText: 'abilityManager',
    kind: 9,
      data: {
        namedExport: 'abilityManager',
        importDeclaration: '@ohos.app.ability.abilityManager.d.ets'
      },
      hasAction: true
    },
    {
      name: 'Ability',
      sortText: '16',
      insertText: 'Ability',
      kind: 7,
      data: {
        namedExport: 'Ability',
        importDeclaration: '@ohos.app.ability.Ability.d.ets'
      },
      hasAction: true
    },
    {
      name: 'AbilityLifecycleCallback',
      sortText: '16',
      insertText: 'AbilityLifecycleCallback',
      kind: 7,
      data: {
        namedExport: 'AbilityLifecycleCallback',
        importDeclaration: '@ohos.app.ability.AbilityLifecycleCallback.d.ets'
      },
      hasAction: true
    }
  ];
  const EXPECT_019 = [
    {
      name: 'accessibility',
      sortText: '16',
      insertText: 'accessibility',
      kind: 9,
      data: {
        namedExport: 'accessibility',
        importDeclaration: '@ohos.accessibility.d.ets'
      },
      hasAction: true
    },
    {
      name: 'AccessibilityExtensionAbility',
      sortText: '16',
      insertText: 'AccessibilityExtensionAbility',
      kind: 7,
      data: {
        namedExport: 'AccessibilityExtensionAbility',
        importDeclaration: '@ohos.application.AccessibilityExtensionAbility.d.ets'
      },
      hasAction: true
    },
    {
      name: 'access',
      sortText: '16',
      insertText: 'access',
      kind: 9,
      data: {
        namedExport: 'access',
        importDeclaration: '@ohos.bluetooth.access.d.ets'
      },
      hasAction: true
    }
  ];
  const EXPECT_020 = [
    {
      name: 'StartOptions',
      sortText: '16',
      insertText: 'StartOptions',
      kind: 7,
      data: {
        namedExport: 'StartOptions',
        importDeclaration: '@ohos.app.ability.StartOptions.d.ets'
      },
      hasAction: true
    },
    {
      name: 'StartupConfig',
      sortText: '16',
      insertText: 'StartupConfig',
      kind: 8,
      data: {
        namedExport: 'StartupConfig',
        importDeclaration: '@ohos.app.appstartup.StartupConfig.d.ets'
      },
      hasAction: true
    },
    {
      name: 'StartupListener',
      sortText: '16',
      insertText: 'StartupListener',
      kind: 7,
      data: {
        namedExport: 'StartupListener',
        importDeclaration: '@ohos.app.appstartup.StartupListener.d.ets'
      },
      hasAction: true
    },
    {
      name: 'startupManager',
      sortText: '16',
      insertText: 'startupManager',
      kind: 9,
      data: {
        namedExport: 'startupManager',
        importDeclaration: '@ohos.app.appstartup.startupManager.d.ets'
      },
      hasAction: true
    }
  ];

  function toMatchObjectUnordered(realValue: LspCompletionInfo | undefined, expect: any, sliceSize: number) {
    for (let i = 0; i < sliceSize; i++) {
      const entry = realValue?.entries[i];
      if (
        entry?.name === expect.name &&
        entry?.sortText === expect.sortText &&
        entry?.insertText === expect.insertText &&
        entry?.kind === expect.kind
      ) {
        return true;
      }
    }
    return false;
  }

  function toMatchData(realValue: LspCompletionInfo | undefined, expect: any, sliceSize: number) {
    for (let i = 0; i < sliceSize; i++) {
      const entry = realValue?.entries[i];
      const hasActionMatched = expect.hasAction === undefined || entry?.hasAction === expect.hasAction;
      if (
        entry?.name === expect.name &&
        entry?.sortText === expect.sortText &&
        entry?.insertText === expect.insertText &&
        entry?.kind === expect.kind &&
        hasActionMatched
      ) {
        if (!entry?.data) {
          continue;
        }
        const namedExportMatched = String(entry.data.namedExport) === expect.data.namedExport;
        const importDeclarationMatched = String(entry.data.importDeclaration).endsWith(expect.data.importDeclaration);
        if (namedExportMatched && importDeclarationMatched) {
          return true;
        }
      }
    }
    return false;
  }

  function expectEntriesContainUnordered(realValue: LspCompletionInfo | undefined, expected: any[]) {
    expect(realValue).toBeDefined();
    expected.forEach((item) => {
      expect(toMatchObjectUnordered(realValue, item, realValue!.entries.length)).toBe(true);
    });
  }

  function expectEntriesContainUnorderedWithData(realValue: LspCompletionInfo | undefined, expected: any[]) {
    expect(realValue).toBeDefined();
    expected.forEach((item) => {
      expect(toMatchData(realValue, item, realValue!.entries.length)).toBe(true);
    });
  }
  describe('No UI Plugins', () => {
    const lsp = getLsp(moduleName);
    test('getCompletionAtPosition_000', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition1.ets'), 705);
      expectEntriesContainUnordered(res, EXPECT_000);
    });
    test('getCompletionAtPosition_001', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition2.ets'), 735);
      expectEntriesContainUnordered(res, EXPECT_001);
    });
    test('getCompletionAtPosition_002', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition3.ets'), 789);
      expectEntriesContainUnordered(res, EXPECT_002);
    });
    test('getCompletionAtPosition_003', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition4.ets'), 767);
      expectEntriesContainUnordered(res, EXPECT_003);
    });
    test('getCompletionAtPosition_004', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition5.ets'), 728);
      expectEntriesContainUnordered(res, EXPECT_004);
    });
    test('getCompletionAtPosition_005', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition6.ets'), 718);
      expectEntriesContainUnordered(res, EXPECT_005);
    });
    test('getCompletionAtPosition_006', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition7.ets'), 683);
      expectEntriesContainUnordered(res, EXPECT_006);
    });
    test('getCompletionAtPosition_007', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition8.ets'), 614);
      expect(res).toBeDefined();
    });
    test('getCompletionAtPosition_008', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition9.ets'), 619);
      expectEntriesContainUnordered(res, EXPECT_008);
    });
    test('getCompletionAtPosition_009', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition10.ets'), 712);
      expectEntriesContainUnordered(res, EXPECT_009);
    });
    test('getCompletionAtPosition_010', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition11.ets'), 682);
      expectEntriesContainUnordered(res, EXPECT_010);
    });
    test('getCompletionAtPosition_011', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition12.ets'), 720);
      expectEntriesContainUnordered(res, EXPECT_011);
    });
    test('getCompletionAtPosition_012', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition13.ets'), 658);
      expectEntriesContainUnordered(res, EXPECT_012);
    });
    test('getCompletionAtPosition_013', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition14.ets'), 659);
      expectEntriesContainUnordered(res, EXPECT_013);
    });
    test('getCompletionAtPosition_015', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition17.ets'), 764);
      expectEntriesContainUnordered(res, EXPECT_015);
    });
    test('getCompletionAtPosition_016', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition17.ets'), 782);
      expectEntriesContainUnordered(res, EXPECT_016);
    });
    test('getCompletionAtPosition_017', () => {
      const res = lsp.getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition18.ets'), 868);
      expectEntriesContainUnordered(res, EXPECT_017);
    });
  });

  describe('With UI Plugins', () => {
    const getUiLsp = (): ReturnType<typeof getLspWithUi> => getLspWithUi(moduleName);
    const collectFilesRecursively = (dirPath: string): string[] => {
      if (!fs.existsSync(dirPath)) {
        return [];
      }
      const entries = fs.readdirSync(dirPath, { withFileTypes: true });
      const files: string[] = [];
      entries.forEach((entry) => {
        const fullPath = path.resolve(dirPath, entry.name);
        if (entry.isDirectory()) {
          files.push(...collectFilesRecursively(fullPath));
        } else if (entry.isFile()) {
          files.push(fullPath);
        }
      });
      return files;
    };
    const kitsDir = path.resolve('test', 'ets', 'static', 'kits');
    const kitFiles = collectFilesRecursively(kitsDir);
    const getUiLspWithKits = (): ReturnType<typeof getLspWithUi> => {
      const lsp = getUiLsp();
      lsp.compileKits(kitFiles);
      return lsp;
    };
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCompletionAtPosition_014', () => {
      const res = getUiLspWithKits().getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition15.ets'), 722);
      expectEntriesContainUnordered(res, EXPECT_014);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCompletionAtPosition_018', () => {
      const res = getUiLspWithKits().getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition19.ets'), 613);
      expectEntriesContainUnorderedWithData(res, EXPECT_018);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCompletionAtPosition_019', () => {
      const res = getUiLspWithKits().getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition20.ets'), 614);
      expectEntriesContainUnorderedWithData(res, EXPECT_019);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('getCompletionAtPosition_020', () => {
      const res = getUiLspWithKits().getCompletionAtPosition(getRealPath(moduleName, 'getCompletionsAtPosition21.ets'), 614);
      expectEntriesContainUnorderedWithData(res, EXPECT_020);
    });
  });
});
