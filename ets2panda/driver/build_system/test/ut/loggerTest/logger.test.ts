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

import {
    ILogger,
    Logger,
    LogDataFactory,
    LogData,
    SubsystemCode,
    getConsoleLogger
} from '../../../src/logger';
import { ErrorCode } from '../../../src/util/error';
import { getMockLoggerGetter } from '../mock/data'

// This test suite is for the Logger class, which handles logging for different subsystems in the build system.
describe('test Logger class', () => {

    beforeEach(() => {
        Logger.getInstance(getMockLoggerGetter());
    });

    afterEach(() => {
        Logger.destroyInstance();
        jest.restoreAllMocks();
    });

    test('singleton', () => {
        Logger.destroyInstance();
        expect(() => Logger.getInstance()).toThrow('loggerGetter is required for the first instantiation.');
        const logger1 = Logger.getInstance(getMockLoggerGetter());
        const logger2 = Logger.getInstance();
        expect(logger1).toBe(logger2);
        const logger3 = Logger.getInstance(getMockLoggerGetter());
        Logger.destroyInstance();
        const logger4 = Logger.getInstance(getMockLoggerGetter());
        expect(logger3).not.toBe(logger4);
    });

    test('consoleLogger', () => {
        getConsoleLogger('TEST')
    });

    test('printInfo', () => {
        const spy: jest.Mock = jest.fn();
        Logger.destroyInstance();
        const logger = Logger.getInstance(getMockLoggerGetter(spy));
        logger.printInfo('info');
        logger.printWarn('warn');
        logger.printDebug('debug');
        expect(spy).toHaveBeenCalledWith('info');
        expect(spy).toHaveBeenCalledWith('warn');
        expect(spy).toHaveBeenCalledWith('debug');
    });

    test('printError && printErrorAndExit', () => {
        const spy: jest.Mock = jest.fn();
        const logger: Logger = Logger.getInstance();

        (logger as any).loggerMap['001' as SubsystemCode] = getMockLoggerGetter(spy)('001' as SubsystemCode);
        (logger as any).loggerMap['002' as SubsystemCode] = getMockLoggerGetter(spy)('002' as SubsystemCode);

        let logData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc');
        logger.printError(logData);
        expect(logger.hasErrors()).toBe(true);
        expect(spy).toHaveBeenCalledWith(logData);

        logData = LogDataFactory.newInstance('00200001' as ErrorCode, 'desc');
        logger.printErrorAndExit(logData);
        expect(logger.hasErrors()).toBe(true);
        expect(spy).toHaveBeenCalledWith(logData);
    });

    test('hasErrors && resetErrorFlag', () => {
        const logger = Logger.getInstance()
        expect(logger.hasErrors()).toBe(false);
        (logger as any).hasErrorOccurred = true;
        expect(logger.hasErrors()).toBe(true);
        logger.resetErrorFlag();
        expect(logger.hasErrors()).toBe(false);
    });

    test('ValidErrorCode', () => {
        const logger = Logger.getInstance()
        expect((logger as any).isValidErrorCode('12345678' as ErrorCode)).toBe(true);
        expect((logger as any).isValidErrorCode('1234567' as ErrorCode)).toBe(false);
        expect((logger as any).isValidErrorCode('abcdefgh' as ErrorCode)).toBe(false);
    });

    test('getLoggerFromSubsystemCode', () => {
        const logger = Logger.getInstance();
        expect(() => { (logger as any).getLoggerFromSubsystemCode('INVALID' as SubsystemCode) }).toThrow('Invalid subsystemCode.');
        const fakeLogger: ILogger = getMockLoggerGetter()('FKLGR' as SubsystemCode);
        (logger as any).loggerMap['FKLGR' as SubsystemCode] = fakeLogger;
        expect((logger as any).getLoggerFromSubsystemCode('FKLGR' as SubsystemCode)).toBe(fakeLogger);
    });

    test('getLoggerFromErrorCode', () => {
        const logger = Logger.getInstance();
        expect(() => (logger as any).getLoggerFromErrorCode('badcode' as ErrorCode)).toThrow('Invalid errorCode.');
        const fakeLogger: ILogger = (getMockLoggerGetter())('001' as SubsystemCode);
        (logger as any).loggerMap['001' as SubsystemCode] = fakeLogger;
        expect((logger as any).getLoggerFromErrorCode('00100001' as ErrorCode)).toBe(fakeLogger);
    });
});

// This test suite is for the LogDataFactory and LogData classes, which are used to create log data instances.
describe('test LogDataFactory and LogData', () => {
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
        logData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc');
        str = logData.toString();
        expect(str).toContain('ERROR Code: 00100001 desc');
        expect(str).not.toContain('Error Message:');
        expect(str).not.toContain('More Info:');
    });
});

describe('test console logger', () => {
    test('print', () => {
        const logger = getConsoleLogger('test');
        const spy = jest.fn()
        global.console.info = spy
        global.console.debug = spy
        global.console.warn = spy
        global.console.error = spy
        global.process.exit = jest.fn() as any

        logger.printInfo('info')
        logger.printDebug('debug')
        logger.printWarn('warn')
        const errorData: LogData = LogDataFactory.newInstance('00100001' as ErrorCode, 'desc');
        const errorDataStr: string = errorData.toString();

        logger.printError(errorData)
        logger.printErrorAndExit(errorData)

        expect(spy).toHaveBeenCalledWith('info');
        expect(spy).toHaveBeenCalledWith('warn');
        expect(spy).toHaveBeenCalledWith('debug');
        expect(spy).toHaveBeenNthCalledWith(4, errorDataStr);
        expect(spy).toHaveBeenNthCalledWith(5, errorDataStr);
    });
});
