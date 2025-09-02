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

import { Logger, LogDataFactory, LogData } from '../../../src/logger';
import { SubsystemCode, ErrorCode } from '../../../src/error_code';
import { BuildConfig, BUILD_TYPE, BUILD_MODE } from '../../../src/types';

function createMockBuildConfig(): BuildConfig {
  return {
    getHvigorConsoleLogger: jest.fn(() => ({
      printInfo: jest.fn(),
      printWarn: jest.fn(),
      printDebug: jest.fn(),
      printError: jest.fn(),
      printErrorAndExit: jest.fn(),
    })),
    packageName: 'mockpkg',
    moduleType: 'shared',
    moduleRootPath: '/mock/module',
    sourceRoots: [],
    byteCodeHar: false,
    plugins: {},
    compileFiles: [],
    dependentModuleList: [],
    buildType: BUILD_TYPE.BUILD,
    buildMode: BUILD_MODE.DEBUG,
    hasMainModule: true,
    arkts: {} as any,
    arktsGlobal: {} as any,
    declgenV1OutPath: undefined,
    declgenV2OutPath: undefined,
    declgenBridgeCodePath: undefined,
    buildSdkPath: '',
    loaderOutPath: '',
    cachePath: '',
    externalApiPaths: [],
    enableDeclgenEts2Ts: false,
    //`as unknown` to satisfy TypeScript type checking
  } as unknown as BuildConfig;
}

// This test suite is for the Logger class, which handles logging for different subsystems in the build system.
describe('test Logger class', () => {
  let logger: Logger;

  beforeEach(() => {
    Logger.destroyInstance();
    logger = Logger.getInstance(createMockBuildConfig());
  });

  afterEach(() => {
    Logger.destroyInstance();
    jest.restoreAllMocks();
  });

  test('singleton', () => {
    Logger.destroyInstance();
    expect(() => Logger.getInstance()).toThrow('projectConfig is required for the first instantiation.');
    const logger1 = Logger.getInstance(createMockBuildConfig());
    const logger2 = Logger.getInstance();
    expect(logger1).toBe(logger2);
    const logger3 = Logger.getInstance(createMockBuildConfig());
    Logger.destroyInstance();
    const logger4 = Logger.getInstance(createMockBuildConfig());
    expect(logger3).not.toBe(logger4);
  });

  test('printInfo', () => {
    const spy = jest.fn();
    (logger as any).loggerMap[SubsystemCode.BUILDSYSTEM].printInfo = spy;
    (logger as any).loggerMap[SubsystemCode.BUILDSYSTEM].printWarn = spy;
    (logger as any).loggerMap[SubsystemCode.BUILDSYSTEM].printDebug = spy;
    logger.printInfo('info');
    logger.printWarn('warn');
    logger.printDebug('debug');
    expect(spy).toHaveBeenCalledWith('info');
    expect(spy).toHaveBeenCalledWith('warn');
    expect(spy).toHaveBeenCalledWith('debug');
  });

  test('printError && printErrorAndExit', () => {
    const spy = jest.fn();
    // insert persudo code '001' && '002' into the map, for testing.
    (logger as any).loggerMap['001'] = { printError: spy };
    (logger as any).loggerMap['002'] = { printErrorAndExit: spy };
    let logData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc');
    logger.printError(logData);
    expect((logger as any).hasErrorOccurred).toBe(true);
    expect(spy).toHaveBeenCalledWith(logData);
    logData = LogDataFactory.newInstance('00200001' as ErrorCode, 'desc');
    logger.printErrorAndExit(logData);
    expect((logger as any).hasErrorOccurred).toBe(true);
    expect(spy).toHaveBeenCalledWith(logData);
  });

  test('hasErrors && resetErrorFlag', () => {
    expect(logger.hasErrors()).toBe(false);
    (logger as any).hasErrorOccurred = true;
    expect(logger.hasErrors()).toBe(true);
    logger.resetErrorFlag();
    expect(logger.hasErrors()).toBe(false);
  });

  test('ValidErrorCode', () => {
    expect((logger as any).isValidErrorCode('12345678')).toBe(true);
    expect((logger as any).isValidErrorCode('1234567')).toBe(false);
    expect((logger as any).isValidErrorCode('abcdefgh')).toBe(false);
    expect(() => (logger as any).getLoggerFromSubsystemCode('INVALID')).toThrow('Invalid subsystemCode.');
  });

  test('getLoggerFromSubsystemCode', () => {
    const fakeLogger = { printInfo: jest.fn(), printWarn: jest.fn(),
      printDebug: jest.fn(), printError: jest.fn(), printErrorAndExit: jest.fn() };
    (logger as any).loggerMap['FKLGR'] = fakeLogger;
    expect((logger as any).getLoggerFromSubsystemCode('FKLGR')).toBe(fakeLogger);
  });

  test('getLoggerFromErrorCode', () => {
    expect(() => (logger as any).getLoggerFromErrorCode('badcode')).toThrow('Invalid errorCode.');
    const fakeLogger = {
      printInfo: jest.fn(),
      printWarn: jest.fn(),
      printDebug: jest.fn(),
      printError: jest.fn(),
      printErrorAndExit: jest.fn(),
    };
    (logger as any).loggerMap['001'] = fakeLogger;
    expect((logger as any).getLoggerFromErrorCode('00100001')).toBe(fakeLogger);
  });
});

// This test suite is for the LogDataFactory and LogData classes, which are used to create log data instances.
describe('test LogDataFactory && LogData', () => {
  test('LogDataFactory.newInstance creates LogData', () => {
    let logData = LogDataFactory.newInstance(
      '00100001' as ErrorCode, 'desc', 'cause', 'pos', ['sol1', 'sol2'], { foo: 'bar' });
    expect(logData).toBeInstanceOf(LogData);
    expect(logData.code).toBe('00100001');
    expect(logData.description).toBe('desc');
    expect(logData.cause).toBe('cause');
    expect(logData.position).toBe('pos');
    expect(logData.solutions).toEqual(['sol1', 'sol2']);
    expect(logData.moreInfo).toEqual({ foo: 'bar' });
    logData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc', 'cause', 'pos', ['sol1'], { foo: 'bar' });
    let str = logData.toString();
    expect(str).toContain('ERROR Code: 00100001 desc');
    expect(str).toContain('Error Message: cause pos');
    expect(str).toContain('> sol1');
    expect(str).toContain('More Info:');
    expect(str).toContain('FOO: bar');
    logData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc', '', '', [''], undefined);
    str = logData.toString();
    expect(str).toContain('ERROR Code: 00100001 desc');
    expect(str).not.toContain('Error Message:');
    expect(str).not.toContain('More Info:');
  });
});
