/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import { strict as assert } from 'assert';
import fs from 'fs';
import os from 'os';
import path from 'path';
import * as ts from 'typescript';
import { ApiExtractor } from '../../../src/common/ApiExtractor';
import { initScanProjectConfig, readProjectPropertiesByCollectedPaths } from '../../../src/common/ApiReader';
import { ArkObfuscator, clearGlobalCaches } from '../../../src/ArkObfuscator';
import { IOptions } from '../../../src/configs/IOptions';
import { NameGeneratorType } from '../../../src/generator/NameFactory';
import { initObfuscationConfig } from '../../../src/initialization/Initializer';
import { generateobfLoggerPrinter } from '../../../src/initialization/ConfigResolver';
import {
  BytecodeObfuscationCollections,
  PropCollections,
  UnobfuscationCollections
} from '../../../src/utils/CommonCollections';
import { enumPropsSet, objectPropsSet, stringPropsSet, structPropsSet } from '../../../src/utils/OhsUtil';
import {
  clearProjectWhiteListManager,
  initProjectWhiteListManager,
  projectWhiteListManager
} from '../../../src/utils/ProjectCollections';
import renameProperties from '../../../src/transformers/rename/RenamePropertiesTransformer';

describe('Monitor project property preservation', () => {
  let directory: string;
  let options: IOptions;
  let messages: string[];
  let originalWarn: typeof console.warn;

  function clearState(): void {
    clearGlobalCaches();
    clearProjectWhiteListManager();
    structPropsSet.clear();
    stringPropsSet.clear();
    enumPropsSet.clear();
    objectPropsSet.clear();
    ApiExtractor.mPropertySet.clear();
    ApiExtractor.mSystemExportSet.clear();
    initScanProjectConfig({});
    ArkObfuscator.isKeptCurrentFile = false;
  }

  beforeEach(() => {
    clearState();
    directory = fs.mkdtempSync(path.join(os.tmpdir(), 'arkguard-monitor-'));
    options = {
      mNameObfuscation: {
        mEnable: true,
        mRenameProperties: true,
        mKeepStringProperty: false,
        mNameGeneratorType: NameGeneratorType.ORDERED,
        mReservedProperties: []
      },
      mUnobfuscationOption: { mPrintKeptNames: true, mPrintPath: '' }
    };
    messages = [];
    originalWarn = console.warn;
    console.warn = (message: string): void => { messages.push(message); };
  });

  afterEach(() => {
    console.warn = originalWarn;
    clearState();
    fs.rmSync(directory, { recursive: true, force: true });
  });

  function write(name: string, content: string): string {
    const file = path.join(directory, name);
    fs.writeFileSync(file, content);
    return file;
  }

  function scan(files: string[], incremental: boolean = false, cache: string = directory): void {
    initProjectWhiteListManager(cache, incremental, false, false);
    const reserved = readProjectPropertiesByCollectedPaths(new Set(files), options, false);
    new ArkObfuscator().addReservedSetForPropertyObf(reserved);
  }

  function transform(file: string): string {
    const source = ts.createSourceFile(file, fs.readFileSync(file, 'utf8'), ts.ScriptTarget.ES2015,
      true, undefined, { etsAnnotationsEnable: true }, true);
    const factory = renameProperties.transformerPlugin.createTransformerFactory(options);
    const result = ts.transform(source, [factory!]);
    try {
      return ts.createPrinter().printFile(result.transformed[0] as ts.SourceFile);
    } finally {
      result.dispose();
    }
  }

  function initializeLogger(logger: Function, enabled: boolean = true): void {
    const rules = write('obfuscation-rules.txt', '-enable-property-obfuscation');
    initObfuscationConfig({
      compileHar: false,
      obfuscationOptions: {
        selfConfig: { ruleOptions: { enable: enabled, rules: [rules] }, consumerRules: [] },
        dependencies: { libraries: [], hars: [] }
      }
    }, {}, logger);
  }

  it('routes repeated scans through the build logger at warn level and releases it on cleanup', () => {
    const calls: unknown[][] = [];
    initializeLogger((...args: unknown[]) => { calls.push(args); });
    const file = write('logger.ets', "class Model { @Monitor('known.leaf', getPath()) changed() {} }");
    scan([file]);
    scan([file]);
    assert.equal(calls.length, 2);
    const message = calls[0][0] as string;
    assert.deepEqual(calls[0], [message, message, 'warn']);
    assert.deepEqual(calls[1], calls[0]);
    assert.ok(message.includes(`${file}:1:`));
    assert.match(message, /MONITOR_UNSUPPORTED_EXPRESSION/);
    assert.match(message, /@Monitor "getPath\(\)"/);
    assert.match(message, /-keep-property-name/);
    assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['known', 'leaf']);
    assert.deepEqual(messages, []);

    clearGlobalCaches();
    scan([file]);
    assert.equal(calls.length, 2);
    assert.deepEqual(messages, [message]);
  });

  it('replaces the previous project logger and clears it when obfuscation is disabled', () => {
    const firstMessages: string[] = [];
    const secondMessages: string[] = [];
    const file = write('projects.ets', 'class Model { @SyncMonitor(getPath()) changed() {} }');
    initializeLogger((message: string) => { firstMessages.push(message); });
    scan([file]);
    initializeLogger((message: string) => { secondMessages.push(message); });
    scan([file]);
    assert.equal(firstMessages.length, 1);
    assert.deepEqual(secondMessages, firstMessages);
    assert.deepEqual(messages, []);

    initializeLogger(() => { assert.fail('Disabled initialization must not install a logger'); }, false);
    // A subsequent standalone scan must not call either project's logger.
    scan([file]);
    assert.equal(firstMessages.length, 1);
    assert.equal(secondMessages.length, 1);
    assert.deepEqual(messages, secondMessages);
  });

  for (const useNewLogger of [false, true]) {
    it(`uses the configured warning sink without console output (newLogger=${useNewLogger})`, () => {
      const warnings: string[] = [];
      const logger = useNewLogger ? {
        printError: () => { assert.fail('Expected warn level'); },
        printWarn: (message: string) => { warnings.push(message); }
      } : {
        error: () => { assert.fail('Expected warn level'); },
        warn: (_color: string, message: string) => { warnings.push(message); }
      };
      initializeLogger(generateobfLoggerPrinter(logger));
      const file = write('adapter.ets', "class Model { @Monitor('invalid..path') changed() {} }");
      scan([file]);
      assert.equal(warnings.length, 1);
      assert.match(warnings[0], /MONITOR_INVALID_PATH/);
      assert.deepEqual(messages, []);
    });
  }

  for (const keepStrings of [true, false]) {
    it(`preserves paths and references without keeping unrelated members (keepStrings=${keepStrings})`, () => {
      options = { ...options, mNameObfuscation: { ...options.mNameObfuscation!, mKeepStringProperty: keepStrings } };
      const file = write('model.ets', `@ObservedV2 class Model {
        @Trace user = { name: 'before', unobserved: 0 };
        @Trace unused = 0;
        @Monitor('user.name') changed(monitor: IMonitor) {
          this.user.name = 'after'; this['user']['name'] = 'again';
          monitor.value('user.name'); monitor.dirty.includes('user.name');
        }
      }`);
      scan([file]);
      assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['name', 'user']);
      const fileProperties = projectWhiteListManager!.getFileWhiteListMap().get(file)!.fileKeepInfo.structProperties;
      assert.deepEqual([...fileProperties].sort(), ['name', 'user']);
      // A previous mapping must not override a newly discovered reservation.
      PropCollections.historyMangledTable = new Map([['user', 'oldUser'], ['name', 'oldName']]);
      UnobfuscationCollections.reservedSdkApiForProp = new Set(['value', 'dirty', 'includes']);
      const output = transform(file);
      assert.match(output, /@Monitor\('user.name'\)/);
      assert.match(output, /this\.user\.name/);
      assert.match(output, /this\[['"]user['"]\]\[['"]name['"]\]/);
      assert.match(output, /\.value\('user.name'\)/);
      for (const name of ['changed', 'unused', 'unobserved']) {
        assert.ok(PropCollections.globalMangledTable.has(name), `${name} should still be obfuscated`);
      }
      for (const name of ['user', 'name']) {
        assert.ok(UnobfuscationCollections.unobfuscatedPropMap.get(name)?.has('struct'));
        assert.ok(!PropCollections.globalMangledTable.has(name));
      }
      assert.deepEqual(messages, []);
    });
  }

  it('preserves nested names across files regardless of scan order', () => {
    const nested = write('nested.ts', 'class Details { leaf = 0; unrelated = 1; }');
    const owner = write('owner.ets', "@ObservedV2 class Owner { @SyncMonitor('details.leaf') changed() {} }");
    for (const files of [[nested, owner], [owner, nested]]) {
      clearState();
      scan(files);
      const output = transform(nested);
      assert.match(output, /leaf = 0/);
      assert.ok(PropCollections.globalMangledTable.has('unrelated'));
      assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['details', 'leaf']);
    }
  });

  it('preserves only concrete wildcard segments and keeps the options object intact', () => {
    const file = write('wildcard.ets', `class Model {
      person = { address: { child: 0 } }; arr = [];
      @Monitor({ enableWildcard: true }, 'person.address.*') changed() {}
      @SyncMonitor('arr.1.*') syncChanged() {}
    }`);
    scan([file]);
    assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['address', 'arr', 'person']);
    const output = transform(file);
    assert.match(output, /enableWildcard: true/);
    assert.match(output, /'person.address.\*'/);
    assert.match(output, /'arr.1.\*'/);
    assert.ok(PropCollections.globalMangledTable.has('child'));
    assert.ok(!PropCollections.globalMangledTable.has('enableWildcard'));
  });

  it('collects nested paths in ComponentV2 structs in addition to existing struct reservations', () => {
    const file = write('component.ets', `@ComponentV2 struct Component {
      @Local details = new Details();
      @Monitor('details.leaf') changed() {}
      build() {}
    }
    @ObservedV2 class Details { @Trace leaf = 0; unused = 1; }`);
    scan([file]);
    assert.ok(UnobfuscationCollections.reservedStruct.has('leaf'));
    assert.ok(UnobfuscationCollections.reservedStruct.has('details'));
    assert.ok(!UnobfuscationCollections.reservedStruct.has('unused'));
    const output = transform(file);
    assert.match(output, /leaf = 0/);
    assert.ok(PropCollections.globalMangledTable.has('unused'));
  });

  it('keeps numeric index tokens out of the reservation set', () => {
    const file = write('arrays.ets', `@ObservedV2 class Model {
      @Trace matrix = [[1]]; @Trace items = [{ leaf: 0 }];
      @Monitor('matrix.0.0', 'items.0.leaf', 'items.length') changed() {}
      read() { return this.matrix[0][0] + this.items[0].leaf; }
    }`);
    scan([file]);
    assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['items', 'leaf', 'length', 'matrix']);
    const output = transform(file);
    assert.match(output, /this\.matrix\[0\]\[0\]/);
    assert.match(output, /this\.items\[0\]\.leaf/);
    assert.ok(!PropCollections.globalMangledTable.has('0'));
  });

  it('reports unknown paths while preserving independently resolvable paths', () => {
    const file = write('unknown.ets', "class Model { @Monitor('known.leaf', getPath()) changed() {} }");
    scan([file]);
    assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), ['known', 'leaf']);
    assert.equal(messages.length, 1);
    assert.ok(messages[0].includes(file));
    assert.match(messages[0], /MONITOR_UNSUPPORTED_EXPRESSION/);
    assert.match(messages[0], /getPath\(\)/);
  });

  for (const disabled of ['properties', 'names']) {
    it(`does not collect or diagnose when ${disabled} obfuscation is disabled`, () => {
      const file = write('disabled.ets', "class Model { @Monitor('known', getPath()) changed() {} }");
      options = { ...options, mExportObfuscation: true, // Still take the PROJECT scanning path.
        mNameObfuscation: { ...options.mNameObfuscation!,
          mRenameProperties: disabled !== 'properties', mEnable: disabled !== 'names' } };
      scan([file]);
      assert.equal(UnobfuscationCollections.reservedStruct.size, 0);
      assert.equal(projectWhiteListManager!.getFileWhiteListMap().get(file)!.fileKeepInfo.structProperties.size, 0);
      assert.deepEqual(messages, []);
    });
  }

  it('does not add Monitor reservations in SDK/API or bytecode-only scanning', () => {
    const file = write('api.ets', "class Model { @Monitor('known', getPath()) changed() {} }");
    initScanProjectConfig(options);
    ApiExtractor.traverseApiFiles(file, ApiExtractor.ApiType.API);
    assert.equal(structPropsSet.size, 0);
    assert.deepEqual(messages, []);
    options = { ...options, mNameObfuscation: { ...options.mNameObfuscation!, mRenameProperties: false } };
    initProjectWhiteListManager(directory, false, false, false);
    readProjectPropertiesByCollectedPaths(new Set([file]), options, false, true);
    assert.equal(structPropsSet.size, 0);
    assert.deepEqual(messages, []);
    assert.ok(BytecodeObfuscationCollections.decoratorProp.has('changed'));
    // Keep the legacy bytecode collection unchanged, including its argument handling.
    assert.deepEqual([...BytecodeObfuscationCollections.decoratorProp].sort(), ['changed', 'getPath()', 'known']);
  });

  it('rebuilds the reservation union after additions, changes and deletions', () => {
    const data = write('data.ts', 'class Data { user = { name: 0, age: 0 }; }');
    const listener = path.join(directory, 'listener.ets');
    const other = path.join(directory, 'other.ets');
    function round(files: string[], expected: string[], changed: boolean, deleted: string[] = []): void {
      clearState();
      scan(files, true);
      projectWhiteListManager!.createOrUpdateWhiteListCaches(new Set(deleted));
      assert.equal(projectWhiteListManager!.getShouldReObfuscate(), changed);
      assert.deepEqual([...UnobfuscationCollections.reservedStruct].sort(), expected.slice().sort());
      const project = JSON.parse(fs.readFileSync(projectWhiteListManager!.getProjectWhiteListCachePath(), 'utf8'));
      assert.deepEqual(project.projectKeepInfo.propertyNames.slice().sort(), expected.slice().sort());
      const output = transform(data);
      for (const name of ['user', 'name', 'age']) {
        assert.equal(PropCollections.globalMangledTable.has(name), !expected.includes(name), output);
      }
    }
    scan([data]);
    projectWhiteListManager!.createOrUpdateWhiteListCaches();
    write('listener.ets', "class Owner { @Monitor('user.name') changed() {} }");
    round([listener], ['user', 'name'], true);
    round([], ['user', 'name'], false); // Recover reservations from unchanged files.
    write('other.ets', "class Other { @SyncMonitor('user.name') changed() {} }");
    round([other], ['user', 'name'], false);
    write('listener.ets', "class Owner { @Monitor('user.age') changed() {} }");
    round([listener], ['user', 'name', 'age'], true);
    fs.unlinkSync(other);
    round([], ['user', 'age'], true, [other]);
    write('listener.ets', 'class Owner { changed() {} }');
    round([listener], [], true);
    fs.unlinkSync(listener);
    round([], [], false, [listener]);
    const cached = fs.readFileSync(projectWhiteListManager!.getProjectWhiteListCachePath(), 'utf8');
    const cleanCache = path.join(directory, 'clean');
    fs.mkdirSync(cleanCache);
    clearState();
    scan([data], false, cleanCache);
    projectWhiteListManager!.createOrUpdateWhiteListCaches();
    assert.deepEqual(JSON.parse(fs.readFileSync(projectWhiteListManager!.getProjectWhiteListCachePath(), 'utf8')),
      JSON.parse(cached));
  });

  it('retains multiple keep reasons and does not leak reservations to the next project', () => {
    const file = write('reason.ets', "class Model { name = 0; @Monitor('name') changed() {} }");
    scan([file]);
    PropCollections.reservedProperties.add('name');
    transform(file);
    assert.deepEqual([...UnobfuscationCollections.unobfuscatedPropMap.get('name')!].sort(), ['conf', 'struct']);
    clearState();
    const next = write('next.ts', 'class Next { name = 0; }');
    scan([next]);
    transform(next);
    assert.ok(PropCollections.globalMangledTable.has('name'));
    assert.equal(UnobfuscationCollections.reservedStruct.size, 0);
  });
});
