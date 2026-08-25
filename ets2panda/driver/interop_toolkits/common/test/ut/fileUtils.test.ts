/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import path from 'node:path';

import { INTERNAL_PREFIX, normalizePath, toPlatformPath } from '../../src/fileUtils';

describe('fileUtils', () => {
  it('normalizes file paths to absolute paths with forward slashes', () => {
    expect(normalizePath('src/Index.ets')).toBe(path.resolve('src/Index.ets').replace(/\\/g, '/'));
  });

  it('preserves internal virtual paths', () => {
    const internalPath = `${INTERNAL_PREFIX}/lib.es2021.d.ts`;

    expect(normalizePath(internalPath)).toBe(internalPath);
  });

  it('converts path separators to the platform separator', () => {
    expect(toPlatformPath('src\\feature/Index.ets')).toBe(['src', 'feature', 'Index.ets'].join(path.sep));
  });
});
