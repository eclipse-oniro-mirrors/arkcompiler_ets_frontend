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

import { promises as fs } from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import {
  hasEtsSourceExtension,
  hasUseStaticDirective,
  hasUseStaticDirectiveInFile,
} from '../../src/utils/staticSource';

describe('static source rules', () => {
  it.each([
    ['Index.ets', true],
    ['Types.d.ets', true],
    ['Index.ETS', false],
    ['Index.ts', false],
  ])('classifies the ETS extension of %s', (filePath, expected) => {
    expect(hasEtsSourceExtension(filePath)).toBe(expected);
  });

  it.each([
    ["'use static'\nexport const value = true;\n", true],
    ["  'use static'  \r\nexport const value = true;\n", true],
    ['"use static"\n', false],
    ["// comment\n'use static'\n", false],
    ["'use static';\n", false],
    ['', false],
  ])('classifies an ArkTS static source prefix', (sourcePrefix, expected) => {
    expect(hasUseStaticDirective(sourcePrefix)).toBe(expected);
  });

  describe('file probing', () => {
    let rootPath: string;

    beforeEach(async () => {
      rootPath = await fs.mkdtemp(path.join(os.tmpdir(), 'gluegen-static-source-'));
    });

    afterEach(async () => {
      await fs.rm(rootPath, { recursive: true, force: true });
    });

    it('classifies static mode from the beginning of a source file', async () => {
      const staticFile = path.join(rootPath, 'Static.ets');
      const dynamicFile = path.join(rootPath, 'Dynamic.ets');
      await Promise.all([
        fs.writeFile(staticFile, "'use static'\nexport const value = true;\n"),
        fs.writeFile(dynamicFile, 'export const value = true;\n'),
      ]);

      expect(await hasUseStaticDirectiveInFile(staticFile)).toBe(true);
      expect(await hasUseStaticDirectiveInFile(dynamicFile)).toBe(false);
    });

    it('propagates filesystem errors to the caller', async () => {
      await expect(hasUseStaticDirectiveInFile(path.join(rootPath, 'Missing.ets'))).rejects.toMatchObject({
        code: 'ENOENT',
      });
    });
  });
});
