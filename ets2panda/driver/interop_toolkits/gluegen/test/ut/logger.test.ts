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

import { ConsoleLogger, LogData, createRunLogger, type ILogger, type LoggerGetter } from '../../src/logger';
import { GlueGenDiagnosticError, GlueGenErrorCode, errorMessage } from '../../src/errors';

describe('gluegen run logger', () => {
  afterEach(() => {
    jest.restoreAllMocks();
  });

  it('uses the logger returned by the hvigor getter', () => {
    const logger = fakeLogger();
    const getter = jest.fn<ReturnType<LoggerGetter>, Parameters<LoggerGetter>>(() => logger);

    expect(createRunLogger(getter)).toBe(logger);
    expect(getter).toHaveBeenCalledWith('114');
  });

  it('creates an independent console fallback for every run', () => {
    const first = createRunLogger(undefined);
    const second = createRunLogger(undefined);

    expect(first).toBeInstanceOf(ConsoleLogger);
    expect(second).toBeInstanceOf(ConsoleLogger);
    expect(first).not.toBe(second);
  });

  it('propagates an hvigor getter failure', () => {
    const getter: LoggerGetter = () => {
      throw new Error('getter failed');
    };

    expect(() => createRunLogger(getter)).toThrow('getter failed');
  });

  it('creates a LogData value with defaults and stable formatting', () => {
    const minimal = new LogData({
      code: GlueGenErrorCode.INVALID_BUILD_CONFIG,
      description: 'Glue generation failed.',
    });
    expect(minimal).toMatchObject({
      code: '11420001',
      description: 'Glue generation failed.',
      cause: '',
      position: '',
      solutions: [],
    });
    expect(minimal.toString()).toBe('ERROR Code: 11420001 Glue generation failed.\n');

    const detailed = new LogData({
      code: '11503319',
      description: 'Semantic error',
      cause: 'Type mismatch.',
      position: 'Index.ets:1:2',
      solutions: ['Fix the assigned type.'],
      moreInfo: { origin: 'compiler' },
    });
    expect(detailed.toString()).toContain('Error Message: Type mismatch.');
    expect(detailed.toString()).toContain('Position: Index.ets:1:2');
    expect(detailed.toString()).toContain('  > Fix the assigned type.');
    expect(detailed.toString()).toContain('ORIGIN: compiler');

    expect(
      () =>
        new LogData({
          code: 'GLUEGEN_CONFIG',
          description: 'Invalid code.',
        }),
    ).toThrow('Invalid hvigor error code');
  });

  it('keeps structured LogData on gluegen errors', () => {
    const data = new LogData({
      code: GlueGenErrorCode.INVALID_BUILD_CONFIG,
      description: 'Build configuration is invalid.',
      cause: 'A required field is missing.',
    });
    const error = new GlueGenDiagnosticError(data);

    expect(error.message).toBe(data.description);
    expect(error.logData).toBe(data);
  });

  it('extracts Error messages and preserves caller-specific fallbacks', () => {
    expect(errorMessage(new Error('native failed'), 'unknown failure')).toBe('native failed');
    expect(errorMessage('native failed', 'unknown failure')).toBe('unknown failure');
  });

  it('matches the build-system console logger contract', () => {
    const info = jest.spyOn(console, 'info').mockImplementation(() => undefined);
    const warn = jest.spyOn(console, 'warn').mockImplementation(() => undefined);
    const debug = jest.spyOn(console, 'debug').mockImplementation(() => undefined);
    const error = jest.spyOn(console, 'error').mockImplementation(() => undefined);
    const exit = jest.spyOn(process, 'exit').mockImplementation(() => {
      throw new Error('exit requested');
    });
    const logger = new ConsoleLogger();
    const data = new LogData({
      code: GlueGenErrorCode.INVALID_BUILD_CONFIG,
      description: 'Glue generation failed.',
    });

    logger.printInfo('starting');
    logger.printWarn('warning');
    logger.printDebug('details');
    logger.printError(data);
    expect(() => logger.printErrorAndExit(data)).toThrow('exit requested');

    expect(info).toHaveBeenCalledWith('[INFO]', 'starting');
    expect(warn).toHaveBeenCalledWith('[WARN]', 'warning');
    expect(debug).toHaveBeenCalledWith('[DEBUG]', 'details');
    expect(error).toHaveBeenCalledTimes(2);
    expect(error).toHaveBeenCalledWith('[ERROR]', data.toString());
    expect(exit).toHaveBeenCalledWith(1);
  });
});

function fakeLogger(): ILogger {
  return {
    printInfo: jest.fn(),
    printWarn: jest.fn(),
    printDebug: jest.fn(),
    printError: jest.fn(),
    printErrorAndExit: jest.fn(),
  };
}
