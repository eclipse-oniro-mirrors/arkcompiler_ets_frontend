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

import { PluginDriver, PluginHook } from '../../../src/plugins/plugins_driver';
import {
  BuildConfig,
  BUILD_MODE,
  BUILD_TYPE,
  OHOS_MODULE_TYPE,
  PluginsConfig
} from '../../../src/types';

jest.mock('../../../src/logger');

const mockConfig: BuildConfig = {
  buildMode: BUILD_MODE.DEBUG,
  compileFiles: ["test.ets"],
  packageName: "test",
  moduleRootPath: "/test/path",
  sourceRoots: ["./"],
  loaderOutPath: "./dist",
  cachePath: "./dist/cache",
  plugins: {},
  buildType: BUILD_TYPE.BUILD,
  hasMainModule: true,
  moduleType: OHOS_MODULE_TYPE.HAR,
  arkts: {} as any,
  arktsGlobal: {} as any,
  enableDeclgenEts2Ts: false,
  byteCodeHar: false,
  declgenV1OutPath: "./dist/declgen",
  declgenV2OutPath: "./dist/declgen/v2",
  buildSdkPath: "./sdk",
  externalApiPaths: [],

  dependentModuleList: [
  ]
} as any;

// The PluginDriver class is responsible for managing and executing plugins in the build system.
describe('test PluginDriver', () => {
  beforeEach(() => {
    PluginDriver.destroyInstance();
    jest.clearAllMocks();
  });
  test('constructor', () => {
    test_construcotr();
  });

  test('getInstance', () => {
    test_getInstance();
  });

  test('destroyInstance', () => {
    test_destroyInstance();
  });

  test('getPluginContext', () => {
    test_getPluginContext();
  });

  test('setArkTSAst and getArkTSAst', () => {
    test_setArkTSAstAndGetArkTSAst();
  });

  test('setArkTSProgram and getArkTSProgram', () => {
    test_setArkTSProgramAndGetArkTSProgram();
  });

  test('setProjectConfig and getProjectConfig', () => {
    test_setProjectConfigAndGetProjectConfig();
  });

  test('setFileManager and getFileManager', () => {
    test_setFileManagerAndGetFileManager();
  });

  test('setContextPtr and getContextPtr', () => {
    test_setContextPtrAndGetContextPtr();
  });

  test('runPluginHook001', () => {
    test_runPluginHook001();
  });

  test('runPluginHook002', () => {
    test_runPluginHook002();
  });

  test('getPlugins', () => {
    test_getPlugins();
  });

  test('initPlugins001', () => {
    test_initPlugins001();
  });

  test('initPlugins002', () => {
    test_initPlugins002();
  });

  test('getSortedPlugins', () => {
    test_getSortedPlugins();
  });
});

function test_getSortedPlugins() {
  const driver = PluginDriver.getInstance();
  let mockPreData: any = {
    name: PluginHook.PARSED,
    parsed: {
      order: 'pre',
      handler: jest.fn()
    }
  };
  let mockPostData: any = {
    name: PluginHook.CHECKED,
    checked: {
      order: 'post',
      handler: jest.fn()
    }
  };
  let mockOtherData: any = {
    name: PluginHook.CHECKED,
    checked: {
      order: undefined,
      handler: jest.fn()
    }
  };
  let mockallPlugins = new Map<string, any>()
  mockallPlugins.set(PluginHook.PARSED, mockPreData);
  mockallPlugins.set(PluginHook.CHECKED, mockPostData);
  mockallPlugins.set(PluginHook.NEW, mockOtherData);
  Reflect.set(driver, 'allPlugins', mockallPlugins);
  expect(() => {
    Reflect.get(driver, 'getSortedPlugins').call(driver, PluginHook.PARSED);
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
  expect(() => {
    Reflect.get(driver, 'getSortedPlugins').call(driver, PluginHook.CHECKED);
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}

function test_initPlugins002() {
  const driver = PluginDriver.getInstance();
  mockConfig.plugins = undefined as any;
  driver.initPlugins(mockConfig);
  expect(() => {
    driver.initPlugins(mockConfig);
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
  expect(() => {
    driver.initPlugins(undefined as any);
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}

function test_getPlugins() {
  const driver = PluginDriver.getInstance();
  const handler = {
    get: function (target: any, prop: any) {
      if (prop === 'getSortedPlugins') {
        return target[prop];
      }
    }
  };
  const proxyInstance = new Proxy(driver, handler);
  let mockData: any = [{
    name: 'mockPlugin',
    handler: jest.fn()
  }];
  const spy = jest.spyOn(proxyInstance, 'getSortedPlugins');
  spy.mockReturnValue(mockData);
  expect(() => {
    driver.runPluginHook(PluginHook.PARSED)
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}

function test_runPluginHook001() {
  const driver = PluginDriver.getInstance();
  expect(() => {
    driver.runPluginHook(PluginHook.PARSED)
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}

function test_runPluginHook002() {
  const driver = PluginDriver.getInstance();
  const handler = {
    get: function (target: any, prop: any) {
      if (prop === 'getPlugins') {
        return target[prop];
      }
    }
  };
  const proxyInstance = new Proxy(driver, handler);
  let mockData: any = [{
    name: 'mockPlugin',
    handler: jest.fn()
  }];
  const spy = jest.spyOn(proxyInstance, 'getPlugins');
  spy.mockReturnValue(mockData);
  expect(() => {
    driver.runPluginHook(PluginHook.PARSED)
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}

function test_setContextPtrAndGetContextPtr() {
  const driver = PluginDriver.getInstance();
  const mockPtr = 124;
  driver.getPluginContext().setContextPtr(mockPtr);
  expect(driver.getPluginContext().getContextPtr()).toBe(mockPtr);
}

function test_setFileManagerAndGetFileManager() {
  const driver = PluginDriver.getInstance();
  driver.getPluginContext().setFileManager(mockConfig);
  expect(driver.getPluginContext().getFileManager()).not.toBe(undefined);
}

function test_setProjectConfigAndGetProjectConfig() {
  const driver = PluginDriver.getInstance();
  const mockProgram = { type: 'mock', body: [] };
  driver.getPluginContext().setProjectConfig(mockProgram);
  expect(driver.getPluginContext().getProjectConfig()).toBe(mockProgram);
}

function test_setArkTSProgramAndGetArkTSProgram() {
  const driver = PluginDriver.getInstance();
  const mockProgram = { type: 'mock', body: [] };
  driver.getPluginContext().setArkTSProgram(mockProgram);
  expect(driver.getPluginContext().getArkTSProgram()).toBe(mockProgram);
}

function test_setArkTSAstAndGetArkTSAst() {
  const driver = PluginDriver.getInstance();
  const mockAst = { type: 'mock', body: [] };
  driver.getPluginContext().setArkTSAst(mockAst);
  expect(driver.getPluginContext().getArkTSAst()).toBe(mockAst);
}

function test_getPluginContext() {
  const driver = PluginDriver.getInstance();
  let context = driver.getPluginContext();
  expect(context).not.toBe(undefined);
}

function test_construcotr() {
  const plugindriver = new PluginDriver();
  expect(plugindriver).not.toBe(undefined);
}

function test_getInstance() {
  const driver = PluginDriver.getInstance();
  expect(driver).toBe(PluginDriver.getInstance());
}

function test_destroyInstance() {
  const driver = PluginDriver.getInstance();
  PluginDriver.destroyInstance();
  expect(driver).not.toBe(PluginDriver.getInstance());
}

function test_initPlugins001() {
  const driver = PluginDriver.getInstance();
  expect(() => {
    driver.initPlugins(mockConfig);
  }).not.toThrow('runPluginHook should not throw an error when no plugins are registered for the hook');
}
