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

import fs from 'fs';
import path from 'path';
import { LspCompletionInfo, LspDiagsNode } from '../../src';
import { getLspWithUi, getRealPath } from '../utils';

describe('importDynamicSdkTest', () => {
  const moduleName: string = 'importDynamicSdk';
  const DIAGNOSTICS_001 = [
    {
      message: `Expected 3 arguments, got 0.`,
      range: { start: { line: 56, character: 1 }, end: { line: 56, character: 6 } }
    },
    {
      message: `No matching call signature for info()`,
      range: { start: { line: 56, character: 1 }, end: { line: 56, character: 6 } }
    }
  ];
  const COMPLETIONS_002 = [
    {
      name: 'setBatteryConfig(sceneName: string, sceneValue: string): number',
      sortText: '17',
      insertText: 'setBatteryConfig()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'getBatteryConfig(sceneName: string): string',
      sortText: '17',
      insertText: 'getBatteryConfig()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'isBatteryConfigSupported(sceneName: string): boolean',
      sortText: '17',
      insertText: 'isBatteryConfigSupported()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'batterySOC(): number',
      sortText: '17',
      insertText: 'batterySOC()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'chargingStatus(): BatteryChargeState',
      sortText: '17',
      insertText: 'chargingStatus()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'healthStatus(): BatteryHealthState',
      sortText: '17',
      insertText: 'healthStatus()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'pluggedType(): BatteryPluggedType',
      sortText: '17',
      insertText: 'pluggedType()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'voltage(): number',
      sortText: '17',
      insertText: 'voltage()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'technology(): string',
      sortText: '17',
      insertText: 'technology()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'batteryTemperature(): number',
      sortText: '17',
      insertText: 'batteryTemperature()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'isBatteryPresent(): boolean',
      sortText: '17',
      insertText: 'isBatteryPresent()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'batteryCapacityLevel(): BatteryCapacityLevel',
      sortText: '17',
      insertText: 'batteryCapacityLevel()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'estimatedRemainingChargeTime(): number',
      sortText: '17',
      insertText: 'estimatedRemainingChargeTime()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'totalEnergy(): number',
      sortText: '17',
      insertText: 'totalEnergy()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'nowCurrent(): number',
      sortText: '17',
      insertText: 'nowCurrent()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'remainingEnergy(): number',
      sortText: '17',
      insertText: 'remainingEnergy()',
      kind: 2,
      data: null,
      hasAction: false
    },
    {
      name: 'BatteryPluggedType',
      sortText: '13',
      insertText: 'BatteryPluggedType',
      kind: 13,
      data: null,
      hasAction: false
    },
    {
      name: 'BatteryChargeState',
      sortText: '13',
      insertText: 'BatteryChargeState',
      kind: 13,
      data: null,
      hasAction: false
    },
    {
      name: 'BatteryHealthState',
      sortText: '13',
      insertText: 'BatteryHealthState',
      kind: 13,
      data: null,
      hasAction: false
    },
    {
      name: 'BatteryCapacityLevel',
      sortText: '13',
      insertText: 'BatteryCapacityLevel',
      kind: 13,
      data: null,
      hasAction: false
    },
    {
      name: 'CommonEventBatteryChangedKey',
      sortText: '13',
      insertText: 'CommonEventBatteryChangedKey',
      kind: 13,
      data: null,
      hasAction: false
    }
  ];
  function toMatchCompletions(realValue: LspCompletionInfo | undefined, expect: any, sliceSize: number) {
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

  function toMatchDiagnostics(expect: any, realValue: LspDiagsNode | undefined, sliceSize: number) {
    for (let i = 0; i < sliceSize; i++) {
      const entry = realValue?.diagnostics[i];
      const rangeMatched =
        entry?.range?.start?.line === expect.range.start.line &&
        entry?.range?.start?.character === expect.range.start.character &&
        entry?.range?.end?.line === expect.range.end.line &&
        entry?.range?.end?.character === expect.range.end.character;
      if (
        entry?.message === expect.message &&
        rangeMatched
      ) {
        return true;
      }
    }
    return false;
  }

  function expectEntriesContainUnorderedWithCompletions(realValue: LspCompletionInfo | undefined, expected: any[]) {
    expect(realValue).toBeDefined();
    expected.forEach((item) => {
      expect(toMatchCompletions(realValue, item, realValue!.entries.length)).toBe(true);
    });
  }

  function expectEntriesContainUnorderedWithDiagnostics(expected: any[], realValue: LspDiagsNode | undefined) {
    expect(realValue).toBeDefined();
    expected.forEach((item) => {
      expect(toMatchDiagnostics(item, realValue, realValue!.diagnostics.length)).toBe(true);
    });
  }

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
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_001', () => {
      const res = getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'importDynamicSdkDiagnostics.ets'));
      expectEntriesContainUnorderedWithDiagnostics(DIAGNOSTICS_001, res);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_002', () => {
      const res = getUiLsp().getDefinitionAtPosition(getRealPath(moduleName, 'importDynamicSdkDefinitions.ets'), 856);
      expect(res?.fileName.valueOf().endsWith('@ohos.batteryInfo.d.ets')).toBe(true);
      expect(res?.start).toBe(55);
      expect(res?.length).toBe(11);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_003', () => {
      const res = getUiLsp().getDefinitionAtPosition(getRealPath(moduleName, 'importDynamicSdkDefinitions.ets'), 876);
      expect(res?.fileName.valueOf().endsWith('@ohos.batteryInfo.d.ets')).toBe(true);
      expect(res?.start).toBe(0);
      expect(res?.length).toBe(0);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_004', () => {
      const res = getUiLsp().getCompletionAtPosition(getRealPath(moduleName, 'importDynamicSdkCompletions.ets'), 1535);
      expectEntriesContainUnorderedWithCompletions(res, COMPLETIONS_002);
    });
  });
});
