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
import { AstNodeType, LspCompletionInfo, LspDiagsNode } from '../../src';
import { getLsp, getLspWithUi, getRealPath } from '../utils';

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
  const DIAGNOSTICS_DYNAMIC_IMPORT = [
    {
      message: `Cannot find imported element 'hilogdynamic'`,
      range: { start: { line: 17, character: 9 }, end: { line: 17, character: 21 } }
    },
    {
      message: `Cannot find imported element 'hilog123'`,
      range: { start: { line: 18, character: 9 }, end: { line: 18, character: 17 } }
    }
  ];
  const NODE_INFOS_INTEROP_DEFINITION = [
    { name: 'hilog', kind: AstNodeType.CLASS_DECLARATION },
    { name: 'getOutputDir', kind: AstNodeType.METHOD_DEFINITION }
  ];
  const COMPLETIONS_002 = [
    {
      name: 'batterySOC: number',
      sortText: '14',
      insertText: 'batterySOC',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'chargingStatus: BatteryChargeState',
      sortText: '14',
      insertText: 'chargingStatus',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'healthStatus: BatteryHealthState',
      sortText: '14',
      insertText: 'healthStatus',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'pluggedType: BatteryPluggedType',
      sortText: '14',
      insertText: 'pluggedType',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'voltage: number',
      sortText: '14',
      insertText: 'voltage',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'technology: string',
      sortText: '14',
      insertText: 'technology',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'batteryTemperature: number',
      sortText: '14',
      insertText: 'batteryTemperature',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'isBatteryPresent: boolean',
      sortText: '14',
      insertText: 'isBatteryPresent',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'batteryCapacityLevel: BatteryCapacityLevel',
      sortText: '14',
      insertText: 'batteryCapacityLevel',
      kind: 10,
      data: null,
      hasAction: false
    },
    {
      name: 'nowCurrent: number',
      sortText: '14',
      insertText: 'nowCurrent',
      kind: 10,
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
      const entries = realValue?.entries ?? [];
      if (toMatchCompletions(realValue, item, entries.length)) {
        return;
      }
      const expectedFields = {
        name: item.name,
        sortText: item.sortText,
        insertText: item.insertText,
        kind: item.kind
      };
      const sameNameEntries = entries.filter((entry) => entry?.name === item.name);
      if (sameNameEntries.length === 0) {
        // The expected entry is missing entirely: show every received name.
        expect(entries.map((entry) => entry?.name)).toContain(item.name);
        return;
      }
      // The name exists but some field differs: diff against the closest candidate so the
      // mismatching fields are visible.
      const matchedFieldCount = (entry: any) =>
        [
          entry?.name === item.name,
          entry?.sortText === item.sortText,
          entry?.insertText === item.insertText,
          entry?.kind === item.kind
        ].filter(Boolean).length;
      const closest = sameNameEntries.reduce((best, entry) =>
        matchedFieldCount(entry) > matchedFieldCount(best) ? entry : best);
      expect(closest).toMatchObject(expectedFields);
    });
  }

  function expectEntriesContainUnorderedWithDiagnostics(expected: any[], realValue: LspDiagsNode | undefined) {
    expect(realValue).toBeDefined();
    expected.forEach((item) => {
      const diagnostics = realValue?.diagnostics ?? [];
      if (toMatchDiagnostics(item, realValue, diagnostics.length)) {
        return;
      }
      const sameMessageDiagnostics = diagnostics.filter((diagnostic) => diagnostic?.message === item.message);
      if (sameMessageDiagnostics.length === 0) {
        // The expected diagnostic is missing entirely: show every received message.
        expect(diagnostics.map((diagnostic) => diagnostic?.message)).toContain(item.message);
        return;
      }
      // The message exists but the range differs: diff against the closest candidate so the
      // mismatching range fields are visible.
      const matchedFieldCount = (diagnostic: any) =>
        [
          diagnostic?.range?.start?.line === item.range.start.line,
          diagnostic?.range?.start?.character === item.range.start.character,
          diagnostic?.range?.end?.line === item.range.end.line,
          diagnostic?.range?.end?.character === item.range.end.character
        ].filter(Boolean).length;
      const closest = sameMessageDiagnostics.reduce((best, diagnostic) =>
        matchedFieldCount(diagnostic) > matchedFieldCount(best) ? diagnostic : best);
      expect(closest).toMatchObject({ message: item.message, range: item.range });
    });
  }

  describe('No UI Plugins', () => {
    // These cases build the AST cache (initAstCache), which resolves the fixture's
    // dynamic SDK declarations. Environments whose ets/dynamic fixture cannot be
    // parsed (gate fixture builds) run with SKIP_UI_PLUGINS and skip them.
    let lsp: ReturnType<typeof getLsp> | undefined;
    const getSdkLsp = (): ReturnType<typeof getLsp> => {
      if (lsp === undefined) {
        lsp = getLsp(moduleName);
        lsp.initAstCache();
      }
      return lsp;
    };
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_007', () => {
      const res = getSdkLsp().getCompletionAtPosition(getRealPath(moduleName, 'importDynamicSdkDynamicAutoImport.ets'), 627);
      const entries = (res?.entries ?? []).filter((entry) => entry.name === 'hilog');
      const importDeclarations = entries.map((entry) => String(entry.data?.importDeclaration ?? ''));
      // hilog is suggested once per import form: the dynamic and static canonical roots
      // plus the kit re-export of each root. The array comparison keeps the failure
      // output listing every actually received import declaration when the forms drift.
      expect([...importDeclarations].sort()).toEqual(
        ['@kit.PerformanceAnalysisKit', '@ohos.hilog', 'dynamic@kit.PerformanceAnalysisKit', 'dynamic@ohos.hilog']
      );
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_008', () => {
      const fileName = getRealPath(moduleName, 'importDynamicSdkAdditionalMessage.ets');
      const source = fs.readFileSync(path.resolve(fileName.valueOf()), 'utf-8');
      const offset = source.indexOf('hilog');
      const res = getSdkLsp().getCodeFixesAtPosition(fileName, offset, offset + 'hilog'.length, [1005]);
      expect(res).toBeDefined();
      const actions = res ?? [];
      // The import specifier keeps the module form (dynamic@ohos.hilog / @ohos.hilog) in the
      // description, while additionalMessage reports the declaration file path in both cases.
      const dynamicAction = actions.find((action) => action.description.valueOf().includes("from 'dynamic@ohos.hilog'"));
      const staticAction = actions.find(
        (action) => action.description.valueOf().includes("from '@ohos.hilog'") &&
          !action.description.valueOf().includes('dynamic@')
      );
      expect(dynamicAction).toBeDefined();
      expect(staticAction).toBeDefined();
      // The dynamic declaration comes from the dynamic SDK root
      // (<sdk>/../dynamic/build-tools/interop/<category>/@x.d.ets), not the legacy static
      // root's interop/declaration directory.
      expect(String(dynamicAction!.additionalMessage_).endsWith('@ohos.hilog.d.ets')).toBe(true);
      expect(String(dynamicAction!.additionalMessage_).includes('interop')).toBe(true);
      expect(String(dynamicAction!.additionalMessage_).includes('declaration')).toBe(false);
      expect(String(staticAction!.additionalMessage_).endsWith('@ohos.hilog.d.ets')).toBe(true);
      expect(String(staticAction!.additionalMessage_).includes(`${path.sep}static${path.sep}api${path.sep}`)).toBe(true);
    });

    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_009', () => {
      const fileName = getRealPath(moduleName, 'importDynamicSdkAdditionalMessage.ets');
      const source = fs.readFileSync(path.resolve(fileName.valueOf()), 'utf8');
      const offset = source.indexOf('hilog');
      const res = getSdkLsp().getCodeFixesAtPosition(fileName, offset, offset + 'hilog'.length, [1005]);
      expect(res).toBeDefined();
      // hilog is default-exported by both canonical roots (default import form) and
      // re-exported as a named export by the kit: one suggestion per import form. The
      // array comparison keeps the failure output listing every actually received
      // suggestion when the forms drift.
      const descriptions = (res ?? []).map((action) => String(action.description.valueOf())).sort();
      expect(descriptions).toEqual([
        "Add import hilog from '@ohos.hilog'",
        "Add import hilog from 'dynamic@ohos.hilog'",
        "Add import {hilog} from '@kit.PerformanceAnalysisKit'",
        "Add import {hilog} from 'dynamic@kit.PerformanceAnalysisKit'"
      ]);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_dynamic_sdk_decl_in_rename_locations', () => {
      const fileName = getRealPath(moduleName, 'importDynamicSdkDefinitions.ets');
      const source = fs.readFileSync(path.resolve(fileName.valueOf()), 'utf8');
      const offset = source.indexOf('hilog.info') + 2;
      // The interop declaration of 'dynamic@ohos.hilog' is indexed under the importers'
      // module key (the module context externals resolve the dynamic import), so the
      // rename locations cover its declaration site; every remaining location lives in
      // a project file.
      const locs = getSdkLsp().findRenameLocations(fileName, offset);
      const interopDeclLocs = (locs ?? []).filter(
        (l) =>
          String(l.fileName).includes(`dynamic${path.sep}build-tools${path.sep}interop`) &&
          String(l.fileName).endsWith('@ohos.hilog.d.ets')
      );
      expect(interopDeclLocs.length).toBe(1);
      const projectLocs = (locs ?? []).filter((l) => String(l.fileName).includes('testcases'));
      expect(projectLocs.length).toBeGreaterThan(0);
      expect((locs ?? []).length).toBe(interopDeclLocs.length + projectLocs.length);
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
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_001', () => {
      const res = getUiLsp().getSemanticDiagnostics(getRealPath(moduleName, 'importDynamicSdkDiagnostics.ets'));
      expectEntriesContainUnorderedWithDiagnostics(DIAGNOSTICS_001, res);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_002', () => {
      const res = getUiLsp().getDefinitionAtPosition(getRealPath(moduleName, 'importDynamicSdkDefinitions.ets'), 856);
      expect(res?.fileName.valueOf().endsWith('@ohos.batteryInfo.d.ets')).toBe(true);
      expect(res?.start).toBe(986);
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
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_005', () => {
      const res = getUiLsp().getSemanticDiagnostics(
        getRealPath(moduleName, 'importDynamicSdkDynamicImportDiagnostics.ets')
      );
      expectEntriesContainUnorderedWithDiagnostics(DIAGNOSTICS_DYNAMIC_IMPORT, res);
    });
    (process.env.SKIP_UI_PLUGINS ? test.skip : test)('importDynamicSdk_006', () => {
      const res = getUiLsp().getDefinitionAtPosition(
        getRealPath(moduleName, 'importDynamicSdkInteropDefinition.ets'),
        677
      );
      expect(res?.fileName.valueOf().endsWith('@ohos.hilog.d.ets')).toBe(true);
      expect(res?.start).toBe(13905);
      expect(res?.length).toBe(12);
      expect(res?.nodeInfos?.map(({ name, kind }) => ({ name, kind }))).toEqual(NODE_INFOS_INTEROP_DEFINITION);
    });
  });
});
