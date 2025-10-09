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

import { ensurePathExists } from '../../../src/util/utils';
import * as fs from 'fs';
import * as path from 'path';
import {
  ErrorCode,
} from '../../../src/error_code';
import {
  isWindows, isLinux, isMac,
  changeFileExtension, changeDeclgenFileExtension,
  toUnixPath, safeRealpath
} from '../../../src/util/utils';
import { DECL_ETS_SUFFIX } from '../../../src/pre_define';

describe('Check if the path exists. If not, create it and ensure it exists', () => {
  const testDir = path.join(__dirname, 'testDir');

  beforeEach(() => {
    if (fs.existsSync(testDir)) {
      fs.rmdirSync(testDir, { recursive: true });
    }
  });

  afterEach(() => {
    if (fs.existsSync(testDir)) {
      fs.rmdirSync(testDir, { recursive: true });
    }
  });

  test('test ensurePathExists', () => {
    test_ensurePathExists(testDir);
  });
});

describe('utils', () => {
  // Determine which operating system it is.
  describe('isWindows/isLinux/isMac', () => {
    test('should detect Linux', () => {
      expect(isLinux()).toBe(true);
      expect(isWindows()).toBe(false);
      expect(isMac()).toBe(false);
    });
  });

  describe('test if change file extension successfully', () => {
    test('should change extension when originExt is empty', () => {
      expect(changeFileExtension('a/b/c.txt', '.js')).toBe('a/b/c.js');
    });
    test('should change extension when originExt is provided', () => {
      expect(changeFileExtension('a/b/c.txt', '.js', '.txt')).toBe('a/b/c.js');
    });
  });

  describe('test if changeDeclgenFileExtension works correctly', () => {
    test('should use DECL_ETS_SUFFIX branch', () => {
      const file = `foo${DECL_ETS_SUFFIX}`;
      expect(changeDeclgenFileExtension(file, '.ts')).toBe('foo.ts');
    });
    test('should use default branch', () => {
      expect(changeDeclgenFileExtension('foo.ets', '.ts')).toBe('foo.ts');
    });
  });

  describe('test toUnixPath', () => {
    test('should replace backslashes with slashes', () => {
      expect(toUnixPath('a\\b\\c')).toBe('a/b/c');
    });
  });
});

describe('test if the safeRealpath can resolve the path correctly', () => {
  test('test safeRealpath001', () => {
    const testDir = path.join(__dirname);
    const mockLogger = { printInfo: jest.fn(), printError: jest.fn() };

    const result = safeRealpath(testDir, mockLogger as any);
    expect(result).toBe(fs.realpathSync(testDir));
    expect(mockLogger.printError).not.toHaveBeenCalled();
  });

  test('test safeRealpath002', () => {
    const nonExistentPath = path.join(__dirname, 'non-existent-directory');
    const mockLogger = { printInfo: jest.fn(), printError: jest.fn() };
    expect(() => {
      safeRealpath(nonExistentPath, mockLogger as any);
    }).toThrow();
    expect(mockLogger.printError).toHaveBeenCalledWith(
      expect.objectContaining({
        code: ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL,
        description: expect.stringContaining(`Error resolving path "${nonExistentPath}"`)
      })
    );
  });
});

function test_ensurePathExists(testDir: string) {
  expect(fs.existsSync(testDir)).toBe(false);
  ensurePathExists(path.join(testDir, 'file.txt'));
  expect(fs.existsSync(testDir)).toBe(true);
}

describe('test if get interop files\' path by Api', () => {
  const originalFs = require('fs');
  let mockFs: any;
  beforeEach(() => {
    mockFs = {
      ...originalFs,
      existsSync: jest.fn()
    };
    jest.mock('fs', () => mockFs);
    jest.resetModules();
    const utils = require('../../../src/util/utils');
    (global as any).getInteropFilePathByApi = utils.getInteropFilePathByApi;
  });

  afterEach(() => {
    delete (global as any).getInteropFilePathByApi;
    jest.unmock('fs');
    jest.resetModules();
  });

  test('should find file in first path', () => {
    mockFs.existsSync.mockImplementation((filePath: string) => {
      return filePath === path.resolve('/sdk/path1', '@ohos.test.d.ets');
    });
    const result = (global as any).getInteropFilePathByApi('@ohos.test', new Set(['/sdk/path1', '/sdk/path2']));
    expect(result).toBe(path.resolve('/sdk/path1', '@ohos.test.d.ets'));
    expect(mockFs.existsSync).toHaveBeenCalledTimes(1);
  });
});

describe('test if get OhmurlByApi works correctly', () => {
  beforeEach(() => {
    jest.resetModules();
    const utils = require('../../../src/util/utils');
    (global as any).getOhmurlByApi = utils.getOhmurlByApi;
    (global as any).NATIVE_MODULE = require('../../../src/pre_define').NATIVE_MODULE;
    (global as any).ARKTS_MODULE_NAME = require('../../../src/pre_define').ARKTS_MODULE_NAME;
    (global as any).sdkConfigPrefix = require('../../../src/pre_define').sdkConfigPrefix;
  });

  afterEach(() => {
    delete (global as any).getOhmurlByApi;
    delete (global as any).NATIVE_MODULE;
    delete (global as any).ARKTS_MODULE_NAME;
    delete (global as any).sdkConfigPrefix;
    jest.resetModules();
  });

  test('should handle native module correctly', () => {
    (global as any).NATIVE_MODULE = new Set(['ohos.test']);
    const result = (global as any).getOhmurlByApi('@ohos.test');
    expect(result).toBe('@ohos:test');
  });

  test('should handle arkts module correctly', () => {
    (global as any).ARKTS_MODULE_NAME = 'arkts';
    const result = (global as any).getOhmurlByApi('@arkts.test');
    expect(result).toBe('@ohos:arkts.test');
  });

  test('should handle regular ohos module correctly', () => {
    const result = (global as any).getOhmurlByApi('@ohos.regular');
    expect(result).toBe('@ohos:regular');
  });

  test('should return empty string for non-matching API format', () => {
    const result = (global as any).getOhmurlByApi('invalid-format');
    expect(result).toBe('');
  });

  test('should handle API with whitespace correctly', () => {
    const result = (global as any).getOhmurlByApi('@ohos.test');
    expect(result).toBe('@ohos:test');
  });
});
