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

import { PathConfig } from '../../src';
import { generateBuildConfigs, ModuleDescriptor } from '../../src/lsp/generateBuildConfig';
import path from 'path';

describe('generateBuildConfigs test', () => {
  const pathConfig: PathConfig = {
    buildSdkPath: '',
    projectPath: path.resolve('./test/ut/mock_hap/test_without_dependency'),
    declgenOutDir: ''
  };
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: path.resolve('./test/ut/mock_hap/test_without_dependency/entry') }
  ];
  test('test without dependency', () => {
    const buildConfigs = generateBuildConfigs(pathConfig, modules);
    expect(buildConfigs['entry'].dependencies).toStrictEqual([]);
  });

  test('test with non-directory module path', () => {
    const filePath = __filename;
    const buildConfigs = generateBuildConfigs(pathConfig, [{ name: 'entry', moduleType: 'har', srcPath: filePath }]);
    expect(buildConfigs['entry'].compileFiles).toStrictEqual([]);
  });

  test('test module properties pass-through', () => {
    const buildConfigs = generateBuildConfigs(pathConfig, [
      {
        name: 'entry',
        moduleType: 'har',
        srcPath: path.resolve('./test/ut/mock_hap/test_without_dependency/entry'),
        permissions: ['ohos.permission.INTERNET', 'ohos.permission.ABILITY_BACKGROUND'],
        deviceTypes: ['phone', 'tablet', '2in1'],
        compatibleVersion: '5.0.5(17)'
      }
    ]);
    expect(buildConfigs['entry'].permissions).toStrictEqual(['ohos.permission.INTERNET', 'ohos.permission.ABILITY_BACKGROUND']);
    expect(buildConfigs['entry'].deviceTypes).toStrictEqual(['phone', 'tablet', '2in1']);
    expect(buildConfigs['entry'].compatibleVersion).toStrictEqual('5.0.5(17)');
  });

  test('test module properties default to undefined when absent', () => {
    const buildConfigs = generateBuildConfigs(pathConfig, modules);
    expect(buildConfigs['entry'].permissions).toBeUndefined();
    expect(buildConfigs['entry'].deviceTypes).toBeUndefined();
    expect(buildConfigs['entry'].compatibleVersion).toBeUndefined();
  });
});

describe('generateBuildConfigs test', () => {
  const pathConfig: PathConfig = {
    buildSdkPath: '',
    projectPath: path.resolve('./test/ut/mock_hap/test_with_dependency'),
    declgenOutDir: ''
  };
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: path.resolve('./test/ut/mock_hap/test_with_dependency/entry') },
    { name: 'har', moduleType: 'har', srcPath: path.resolve('./test/ut/mock_hap/test_with_dependency/har') }
  ];
  test('test with dependency', () => {
    const buildConfigs = generateBuildConfigs(pathConfig, modules);
    expect(buildConfigs['entry'].dependencies).toStrictEqual(['har']);
  });
});
