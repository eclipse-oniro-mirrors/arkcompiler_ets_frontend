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

import * as path from 'path';
import * as fs from 'fs';
import { safeRealpath } from '../../src/utils';
import { ErrorCode } from '../../src/error_code';
import { Logger } from '../../src/logger';

describe('safeRealpath', () => {
  const tempDir = path.join(__dirname, 'tmp');
  const realDir = path.join(tempDir, 'real');
  const linkDir = path.join(tempDir, 'link');
  const nonExistent = path.join(tempDir, 'does-not-exist');

  const mockLogger = {
    printError: jest.fn()
  } as unknown as Logger;

  beforeAll(() => {
    fs.mkdirSync(tempDir, { recursive: true });
    fs.mkdirSync(realDir, { recursive: true });

    if (!fs.existsSync(linkDir)) {
      fs.symlinkSync(realDir, linkDir, 'dir');
    }
  });

  afterAll(() => {
    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  test('returns real path for existing non-symlink', () => {
    const result = safeRealpath(realDir, mockLogger);
    expect(result).toBe(fs.realpathSync(realDir));
  });

  test('returns resolved path for symlink', () => {
    const result = safeRealpath(linkDir, mockLogger);
    expect(result).toBe(fs.realpathSync(linkDir));
    expect(result).toBe(realDir);
  });

  test('throws error for non-existent path', () => {
    try {
      safeRealpath(nonExistent, mockLogger);
    } catch (e: any) {
      expect(e.code).toBe(ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL);
    }
  });
  
  test('throws error for empty path string', () => {
    try {
      safeRealpath('', mockLogger);
    } catch (e: any) {
      expect(e.code).toBe(ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL);
    }
  });
  
});
