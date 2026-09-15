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

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { Language } from '../../../src/fileUtils';
import {
  createDefaultInteropConfigHost,
  createInteropConfigHost,
  type InteropConfigHost,
} from '../../../src/interop-config/host';

describe('interop config host', () => {
  let tempDir: string;

  beforeEach(() => {
    tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'interop-config-host-'));
  });

  afterEach(() => {
    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  function createSourceFile(content: string): string {
    const filePath = path.join(tempDir, 'source.ets');
    fs.writeFileSync(filePath, content, 'utf8');
    return filePath;
  }

  it('lets provided methods override the default host', async () => {
    const getLanguageFromSourceCode = async (_filePath: string): Promise<Language> => Language.DYNAMIC;
    const host = createInteropConfigHost({ getLanguageFromSourceCode });

    await expect(host.getLanguageFromSourceCode('src/main.ets')).resolves.toBe(Language.DYNAMIC);
  });

  it('falls back to the default implementation when the custom host omits the method', async () => {
    const host = createInteropConfigHost({});
    const filePath = createSourceFile('"use static";\nclass C {}\n');

    await expect(host.getLanguageFromSourceCode(filePath)).resolves.toBe(Language.STATIC);
  });

  it('keeps the default method when the custom host passes explicit undefined', async () => {
    // Simulates a JavaScript caller passing an explicitly undefined method.
    const custom = { getLanguageFromSourceCode: undefined } as unknown as Partial<InteropConfigHost>;
    const host = createInteropConfigHost(custom);
    const filePath = createSourceFile('let x = 1;\n');

    await expect(host.getLanguageFromSourceCode(filePath)).resolves.toBe(Language.DYNAMIC);
  });

  it('classifies source code through the default host', async () => {
    const host = createDefaultInteropConfigHost();

    await expect(host.getLanguageFromSourceCode(createSourceFile('"use static";\n'))).resolves.toBe(Language.STATIC);
    await expect(host.getLanguageFromSourceCode(createSourceFile('"use strict";\n'))).resolves.toBe(Language.DYNAMIC);
    await expect(host.getLanguageFromSourceCode(createSourceFile('let x = 1;\n'))).resolves.toBe(Language.DYNAMIC);
  });
});
