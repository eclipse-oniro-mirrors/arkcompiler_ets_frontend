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

import * as common from '@interop-toolkits/common';
import ts from 'typescript';

import { loadTsCompilerOptions } from '../../src/utils';

describe('loadTsCompilerOptions', () => {
  let tempDir: string;

  beforeEach(() => {
    tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'declgen-utils-test-'));
  });

  afterEach(() => {
    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  function writeTsconfig(content: string): string {
    const tsconfigPath = path.join(tempDir, 'tsconfig.json');
    fs.writeFileSync(tsconfigPath, content, 'utf-8');
    return tsconfigPath;
  }

  it('parses a tsconfig with trailing commas in compilerOptions', () => {
    const tsconfigPath = writeTsconfig(
      ['{', '  "compilerOptions": {', '    "strict": true,', '    "target": "es2017",', '  }', '}'].join('\n'),
    );

    const compilerOptions = loadTsCompilerOptions(tsconfigPath);

    expect(compilerOptions).toBeDefined();
  });

  it('parses a tsconfig with comments', () => {
    const tsconfigPath = writeTsconfig(
      [
        '{',
        '  // line comment',
        '  "compilerOptions": {',
        '    /* block comment */',
        '    "strict": true',
        '  }',
        '}',
      ].join('\n'),
    );

    const compilerOptions = loadTsCompilerOptions(tsconfigPath);

    expect(compilerOptions).toBeDefined();
  });

  it('parses a tsconfig without a compilerOptions section', () => {
    const tsconfigPath = writeTsconfig('{}');

    const compilerOptions = loadTsCompilerOptions(tsconfigPath);

    expect(compilerOptions).toBeDefined();
  });

  it('throws an internal error when the tsconfig is not parseable', () => {
    const tsconfigPath = writeTsconfig('{ "compilerOptions": ');

    expect(() => loadTsCompilerOptions(tsconfigPath)).toThrow(common.errors.InternalError);
    expect(() => loadTsCompilerOptions(tsconfigPath)).toThrow(tsconfigPath);
  });

  it('applies the forced overrides on top of the parsed options', () => {
    const tsconfigPath = writeTsconfig(
      ['{', '  "compilerOptions": {', '    "target": "es2015",', '  }', '}'].join('\n'),
    );

    const compilerOptions = loadTsCompilerOptions(tsconfigPath);

    expect(compilerOptions.target).toBe(8);
    expect(compilerOptions.module).toBe(ts.ModuleKind.CommonJS);
  });
});
