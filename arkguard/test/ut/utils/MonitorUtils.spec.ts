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
import * as ts from 'typescript';
import {
  collectMonitorProperties,
  MonitorDiagnosticCode,
  MonitorProperties,
  reportMonitorDiagnostic
} from '../../../src/utils/MonitorUtils';

function collect(source: string): MonitorProperties[] {
  const file = ts.createSourceFile('monitor.ets', source, ts.ScriptTarget.Latest, true);
  const results: MonitorProperties[] = [];
  function visit(node: ts.Node): void {
    const result = collectMonitorProperties(node);
    if (result) {
      results.push(result);
    }
    ts.forEachChild(node, visit);
  }
  visit(file);
  return results;
}

function parse(argumentsText: string, decorator: string = 'Monitor'): MonitorProperties {
  const results = collect(`@ObservedV2 class Model { @${decorator}(${argumentsText}) changed() {} }`);
  assert.equal(results.length, 1);
  return results[0];
}

describe('Monitor literal path collection', () => {
  const paths: [string, string[]][] = [
    ["'name'", ['name']],
    ["'user.address.city', 'region', 'user.address.city'", ['user', 'address', 'city', 'region']],
    ["'info'", ['info']],
    ["'matrix.0.1', 'list.0.name', 'list.length'", ['matrix', 'list', 'name', 'length']],
    ["(('user' + '.name'))", ['user', 'name']],
    ['`user.name`', ['user', 'name']],
    ['`user.${"name"}`', ['user', 'name']],
    ['`arr.${(0)}.name`', ['arr', 'name']],
    ['`arr.${0x10}.name`', ['arr', 'name']],
    ['`flags.${true}.${false}.${null}`', ['flags', 'true', 'false', 'null']],
    ["'用户.姓名', '$model._field'", ['用户', '姓名', '$model', '_field']],
    ["'user\\u002ename'", ['user', 'name']]
  ];
  for (const decorator of ['Monitor', 'SyncMonitor']) {
    for (const [expression, expected] of paths) {
      it(`${decorator} collects ${expression}`, () => {
        const result = parse(expression, decorator);
        assert.deepEqual([...result.properties], expected);
        assert.deepEqual(result.diagnostics, []);
      });
    }
  }

  for (const options of ['{}', '{ enableWildcard: true }', "{ 'enableWildcard': (true) }"]) {
    it(`preserves concrete segments with options ${options}`, () => {
      const result = parse(`${options}, 'person.address.*', 'arr.1.*'`);
      assert.deepEqual([...result.properties], ['person', 'address', 'arr']);
      assert.deepEqual(result.diagnostics, []);
    });
  }

  it('supports SyncMonitor wildcards without options', () => {
    const result = parse("'dateInstance.*', 'mapInstance.*', 'setInstance.*'", 'SyncMonitor');
    assert.deepEqual([...result.properties], ['dateInstance', 'mapInstance', 'setInstance']);
    assert.deepEqual(result.diagnostics, []);
  });

  it('keeps options and paths unchanged', () => {
    const file = ts.createSourceFile('monitor.ets',
      "class Model { @Monitor({enableWildcard: true}, 'user' + '.*') changed() {} }", ts.ScriptTarget.Latest, true);
    const printer = ts.createPrinter();
    const before = printer.printFile(file);
    const visit = (node: ts.Node): void => {
      collectMonitorProperties(node);
      ts.forEachChild(node, visit);
    };
    visit(file);
    assert.equal(printer.printFile(file), before);
  });

  for (const expression of ["'broken.*'", "{enableWildcard: false}, 'broken.*'"]) {
    it(`does not enable wildcard implicitly: ${expression}`, () => {
      const result = parse(expression);
      assert.deepEqual([...result.properties], []);
      assert.equal(result.diagnostics[0].code, MonitorDiagnosticCode.INVALID_PATH);
    });
  }

  for (const path of ['', 'broken..leaf', '.broken', 'broken.', '*.broken', 'broken.*.leaf', '*',
    'broken.**', 'broken[0].leaf', 'broken. leaf', 'broken-name']) {
    it(`rejects the complete path ${JSON.stringify(path)} without partial names`, () => {
      const result = parse(JSON.stringify(path), 'SyncMonitor');
      assert.deepEqual([...result.properties], []);
      assert.equal(result.diagnostics[0].code, MonitorDiagnosticCode.INVALID_PATH);
    });
  }

  for (const expression of ['PATH', 'Paths.NAME', 'getPath()', 'flag ? "one" : "two"',
    '`user.${field}`', 'PREFIX + ".name"', '...paths', '1 + 2', '"arr." + 0', '{}']) {
    it(`diagnoses unknown expression ${expression} independently of other arguments`, () => {
      const result = parse(`'known.leaf', ${expression}`, 'SyncMonitor');
      assert.deepEqual([...result.properties], ['known', 'leaf']);
      assert.equal(result.diagnostics.length, 1);
      assert.equal(result.diagnostics[0].code, MonitorDiagnosticCode.UNSUPPORTED_EXPRESSION);
      assert.equal(result.diagnostics[0].node.getText(), expression);
    });
  }

  for (const options of ['{ other: true }', '{ enableWildcard: flag }', '{ ...options }',
    '{ enableWildcard: true, extra: false }']) {
    it(`diagnoses unsupported options ${options}`, () => {
      const result = parse(`${options}, 'known.leaf'`);
      assert.deepEqual([...result.properties], ['known', 'leaf']);
      assert.equal(result.diagnostics[0].code, MonitorDiagnosticCode.INVALID_OPTIONS);
    });
  }

  it('does not discard subsequent wildcard paths when options are unknown', () => {
    const result = parse("options, 'known.*'");
    assert.deepEqual([...result.properties], ['known']);
    assert.equal(result.diagnostics.length, 1);
    assert.equal(result.diagnostics[0].code, MonitorDiagnosticCode.UNSUPPORTED_EXPRESSION);
  });

  for (const expression of ['', '{}']) {
    it(`diagnoses missing paths in Monitor(${expression})`, () => {
      assert.equal(parse(expression).diagnostics[0].code, MonitorDiagnosticCode.MISSING_PATH);
    });
  }

  it('does not resolve const or enum paths', () => {
    const results = collect(`const PATH = 'name'; enum Paths { NAME = 'name' }
      class Model { @Monitor(PATH, Paths.NAME) changed() {} }`);
    assert.deepEqual([...results[0].properties], []);
    assert.equal(results[0].diagnostics.length, 2);
  });

  it('ignores ordinary calls, aliases, namespaces and unrelated decorators', () => {
    assert.deepEqual(collect(`Monitor('notAPath'); class Model {
      @Alias('alias') a() {} @ArkUI.Monitor('namespace') b() {} @Watch('watch') c() {}
    }`), []);
  });

  for (const declaration of [
    'function Monitor(...args) {}',
    'const Monitor = custom;',
    'const { Monitor } = custom;',
    "import { custom as Monitor } from './custom';",
    "import SyncMonitor from './custom';"
  ]) {
    it(`ignores an explicitly bound decorator: ${declaration}`, () => {
      const name = declaration.includes('SyncMonitor') ? 'SyncMonitor' : 'Monitor';
      assert.deepEqual(collect(`${declaration} class Model { @${name}('custom') changed() {} }`), []);
    });
  }

  it('ignores a decorator shadowed by a function parameter', () => {
    assert.deepEqual(collect("function factory(Monitor) { class Model { @Monitor('custom') changed() {} } }"), []);
  });

  it('collects inherited listeners without keeping callback names', () => {
    const results = collect(`class Base { @Monitor('name') baseChanged() {} }
      class Derived extends Base { @SyncMonitor('name') derivedChanged() {} }`);
    assert.equal(results.length, 2);
    results.forEach(result => assert.deepEqual([...result.properties], ['name']));
  });

  it('reports the file, 1-based position, decorator, expression, code and keep advice', () => {
    const result = parse('getPath()');
    const messages: string[] = [];
    const originalWarn = console.warn;
    console.warn = (message: string): void => { messages.push(message); };
    try {
      reportMonitorDiagnostic(result.decorator, result.diagnostics[0]);
    } finally {
      console.warn = originalWarn;
    }
    assert.equal(messages.length, 1);
    assert.match(messages[0], /monitor\.ets:1:\d+ @Monitor "getPath\(\)"/);
    assert.match(messages[0], /MONITOR_UNSUPPORTED_EXPRESSION/);
    assert.match(messages[0], /-keep-property-name/);
  });
});
