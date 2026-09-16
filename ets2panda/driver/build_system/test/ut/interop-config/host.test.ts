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

import { LANGUAGE_VERSION } from '../../../src/pre_define';
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

  it('lets provided methods override the default host', () => {
    const getLanguageFromSourceCode = (_filePath: string): LANGUAGE_VERSION => LANGUAGE_VERSION.ARKTS_1_1;
    const host = createInteropConfigHost({ getLanguageFromSourceCode });

    expect(host.getLanguageFromSourceCode('src/main.ets')).toBe(LANGUAGE_VERSION.ARKTS_1_1);
  });

  it('falls back to the default implementation when the custom host omits the method', () => {
    const host = createInteropConfigHost({});
    const filePath = createSourceFile("'use static'\nclass C {}\n");

    expect(host.getLanguageFromSourceCode(filePath)).toBe(LANGUAGE_VERSION.ARKTS_1_2);
  });

  it('keeps the default method when the custom host passes explicit undefined', () => {
    // Simulates a JavaScript caller passing an explicitly undefined method.
    const custom = { getLanguageFromSourceCode: undefined } as unknown as Partial<InteropConfigHost>;
    const host = createInteropConfigHost(custom);
    const filePath = createSourceFile('let x = 1;\n');

    expect(host.getLanguageFromSourceCode(filePath)).toBe(LANGUAGE_VERSION.ARKTS_1_1);
  });

  it('classifies source code through the default host', () => {
    const host = createDefaultInteropConfigHost();

    expect(host.getLanguageFromSourceCode(createSourceFile("'use static'\n"))).toBe(LANGUAGE_VERSION.ARKTS_1_2);
    expect(host.getLanguageFromSourceCode(createSourceFile('"use strict";\n'))).toBe(LANGUAGE_VERSION.ARKTS_1_1);
    expect(host.getLanguageFromSourceCode(createSourceFile('let x = 1;\n'))).toBe(LANGUAGE_VERSION.ARKTS_1_1);
  });
});
