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

import { collectFiles, pathExists } from '../../src/utils/fileTree';

describe('arktsconfig file tree', () => {
  let rootPath: string;

  beforeEach(async () => {
    rootPath = await fs.mkdtemp(path.join(os.tmpdir(), 'gluegen-file-tree-'));
  });

  afterEach(async () => {
    await fs.rm(rootPath, { recursive: true, force: true });
  });

  it('collects recursively with an asynchronous predicate', async () => {
    const nestedPath = path.join(rootPath, 'nested');
    await fs.mkdir(nestedPath);
    await Promise.all([
      fs.writeFile(path.join(rootPath, 'Root.ets'), ''),
      fs.writeFile(path.join(rootPath, 'Ignored.ts'), ''),
      fs.writeFile(path.join(nestedPath, 'Nested.d.ets'), ''),
    ]);

    const files = await collectFiles(rootPath, async (filePath) => {
      await Promise.resolve();
      return filePath.endsWith('.ets');
    });

    expect(files.map((filePath) => path.relative(rootPath, filePath)).sort()).toEqual([
      'Root.ets',
      path.join('nested', 'Nested.d.ets'),
    ]);
  });

  it('reports path availability asynchronously', async () => {
    expect(await pathExists(rootPath)).toBe(true);
    expect(await pathExists(path.join(rootPath, 'missing'))).toBe(false);
  });
});
