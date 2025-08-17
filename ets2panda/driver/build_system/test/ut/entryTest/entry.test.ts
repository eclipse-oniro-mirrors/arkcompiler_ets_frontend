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

import * as entryModule from '../../../src/entry';
import {
  BUILD_TYPE,
  BuildConfig,
  OHOS_MODULE_TYPE,
  BUILD_MODE
} from '../../../src/types';
import { Logger } from '../../../src/logger';
import {
  getKoalaModule,
  cleanKoalaModule
} from '../../../src/init/init_koala_modules';

jest.mock('../../../src/build/build_mode');
jest.mock('../../../src/build/build_framework_mode');
jest.mock('../../../src/logger');
jest.mock('../../../src/init/process_build_config', () => ({
  processBuildConfig: jest.fn((config) => config)
}));

beforeEach(() => {
  jest.clearAllMocks();
  process.exit = jest.fn() as any;
  cleanKoalaModule();
});

describe('entry.ts build function with clean', () => {
  function setupLogger(hasErrors = false) {
    Logger.getInstance = jest.fn().mockReturnValue({
      hasErrors: jest.fn().mockReturnValue(hasErrors),
      printInfo: jest.fn(),
      printError: jest.fn()
    });
  }

  test('BUILD_TYPE.BUILD branch cleans and calls BuildMode', async () => {
    const BuildMode = require('../../../src/build/build_mode').BuildMode;
    const mockRun = jest.fn().mockResolvedValue(undefined);
    BuildMode.mockImplementation((config: BuildConfig) => ({
      run: mockRun,
      generateDeclaration: jest.fn()
    }));
    setupLogger();

    const mockConfig = {
      buildType: BUILD_TYPE.BUILD,
      packageName: 'test',
      compileFiles: ['test.ets'],
      enableDeclgenEts2Ts: false,
      frameworkMode: false,
      loaderOutPath: './dist',
      cachePath: './dist/cache',
      moduleType: OHOS_MODULE_TYPE.HAR,
      sourceRoots: ['./'],
      moduleRootPath: '/test/path',
      buildMode: BUILD_MODE.DEBUG
    } as BuildConfig;

    await entryModule.build(mockConfig);

    expect(BuildMode).toHaveBeenCalledWith(expect.objectContaining({
      buildType: BUILD_TYPE.BUILD,
      packageName: 'test'
    }));
    expect(mockRun).toHaveBeenCalled();
    expect(getKoalaModule()).toBeNull();
  });

  test('frameworkMode branch cleans and calls BuildFrameworkMode', async () => {
    const BuildFrameworkMode = require('../../../src/build/build_framework_mode').BuildFrameworkMode;
    const mockRun = jest.fn().mockResolvedValue(undefined);
    BuildFrameworkMode.mockImplementation(() => ({ run: mockRun }));

    setupLogger();

    const mockConfig = {
      buildType: BUILD_TYPE.BUILD,
      packageName: 'test',
      compileFiles: ['test.ets'],
      enableDeclgenEts2Ts: false,
      frameworkMode: true,
      loaderOutPath: './dist',
      cachePath: './dist/cache',
      moduleType: OHOS_MODULE_TYPE.HAR,
      sourceRoots: ['./'],
      moduleRootPath: '/test/path',
      buildMode: BUILD_MODE.DEBUG
    } as BuildConfig;

    await entryModule.build(mockConfig);

    expect(BuildFrameworkMode).toHaveBeenCalledWith(expect.objectContaining({
      frameworkMode: true,
      packageName: 'test'
    }));
    expect(mockRun).toHaveBeenCalled();
    expect(getKoalaModule()).toBeNull();
  });

  test('enableDeclgenEts2Ts branch cleans and calls generateDeclaration', async () => {
    const BuildMode = require('../../../src/build/build_mode').BuildMode;
    const mockGenerateDeclaration = jest.fn().mockResolvedValue(undefined);
    BuildMode.mockImplementation(() => ({ run: jest.fn(), generateDeclaration: mockGenerateDeclaration }));

    setupLogger();

    const mockConfig = {
      buildType: BUILD_TYPE.BUILD,
      packageName: 'test',
      compileFiles: ['test.ets'],
      enableDeclgenEts2Ts: true,
      frameworkMode: false,
      loaderOutPath: './dist',
      cachePath: './dist/cache',
      moduleType: OHOS_MODULE_TYPE.HAR,
      sourceRoots: ['./'],
      moduleRootPath: '/test/path',
      buildMode: BUILD_MODE.DEBUG
    } as BuildConfig;

    await entryModule.build(mockConfig);

    expect(BuildMode).toHaveBeenCalledWith(expect.objectContaining({
      enableDeclgenEts2Ts: true,
      packageName: 'test'
    }));
    expect(mockGenerateDeclaration).toHaveBeenCalled();
    expect(getKoalaModule()).toBeNull();
  });

  test('no matching branch cleans and exits on error', async () => {
    setupLogger(true); // hasErrors = true

    const mockConfig = {
      buildType: BUILD_TYPE.PREVIEW,
      packageName: 'test',
      compileFiles: ['test.ets'],
      enableDeclgenEts2Ts: false,
      frameworkMode: true,
      loaderOutPath: './dist',
      cachePath: './dist/cache',
      moduleType: OHOS_MODULE_TYPE.HAR,
      sourceRoots: ['./'],
      moduleRootPath: '/test/path',
      buildMode: BUILD_MODE.DEBUG
    } as BuildConfig;

    await entryModule.build(mockConfig);

    expect(process.exit).toHaveBeenCalledWith(1);
    expect(getKoalaModule()).toBeNull();
  });
});
