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

import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';
import * as ts from 'typescript';
import { VirtualFileHost, normalizePath, INTERNAL_PREFIX } from '../../../src/dynamic/compiler/virtualFileHost';

function createVirtualHost(): VirtualFileHost {
  return new VirtualFileHost({
    target: ts.ScriptTarget.ES2021,
    module: ts.ModuleKind.CommonJS,
  });
}

describe('VirtualFileHost', () => {
  it('should update and read virtual file content', () => {
    const virtualHost = createVirtualHost();
    const fileName = '/virtual/sample.d.ts';
    const content = 'declare const foo: number;';

    virtualHost.updateFile(fileName, content);

    expect(virtualHost.readFile(fileName)).toBe(content);
    expect(virtualHost.fileExists(fileName)).toBe(true);
  });

  it('should use virtual content first, then fallback to base readFile', () => {
    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'vfh-test-'));
    const baseFileName = path.join(tempDir, 'base.d.ts');
    const overrideFileName = path.join(tempDir, 'override.d.ts');

    fs.writeFileSync(baseFileName, 'declare const fromBase: string;', 'utf8');
    fs.writeFileSync(overrideFileName, 'declare const oldValue: string;', 'utf8');

    const virtualHost = createVirtualHost();
    virtualHost.updateFile(overrideFileName, 'declare const newValue: string;');

    const compilerHost = virtualHost.getCompilerHost();

    expect(compilerHost.readFile(overrideFileName)).toBe('declare const newValue: string;');
    expect(compilerHost.readFile(baseFileName)).toBe('declare const fromBase: string;');

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should check file existence using virtual files and base fallback', () => {
    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'vfh-test-'));
    const baseFileName = path.join(tempDir, 'existing.d.ts');
    const virtualFileName = path.join(tempDir, 'virtual-existing.d.ts');

    fs.writeFileSync(baseFileName, 'declare const baseExists: boolean;', 'utf8');

    const virtualHost = createVirtualHost();
    virtualHost.updateFile(virtualFileName, 'declare const virtualExists: boolean;');

    const compilerHost = virtualHost.getCompilerHost();

    expect(compilerHost.fileExists(virtualFileName)).toBe(true);
    expect(compilerHost.fileExists(baseFileName)).toBe(true);
    expect(compilerHost.fileExists(path.join(tempDir, 'missing-file.d.ts'))).toBe(false);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should create SourceFile from virtual content and fallback to base host', () => {
    const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'vfh-test-'));
    const baseFileName = path.join(tempDir, 'source.d.ts');
    const virtualFileName = path.join(tempDir, 'virtual-source.d.ts');

    fs.writeFileSync(baseFileName, 'declare const fromBase: string;', 'utf8');

    const virtualHost = createVirtualHost();
    virtualHost.updateFile(virtualFileName, 'declare const fromVirtual: number;');

    const compilerHost = virtualHost.getCompilerHost();

    const virtualSource = compilerHost.getSourceFile(virtualFileName, ts.ScriptTarget.ES2021);
    expect(virtualSource).toBeTruthy();
    expect(virtualSource?.text.includes('fromVirtual')).toBe(true);

    const baseSource = compilerHost.getSourceFile(baseFileName, ts.ScriptTarget.ES2021);
    expect(baseSource).toBeTruthy();
    expect(baseSource?.text.includes('fromBase')).toBe(true);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should expose createHash and match ts.sys.createHash', () => {
    const virtualHost = createVirtualHost();
    const input = 'hash-input';
    expect(virtualHost.createHash(input)).toBe(ts.sys.createHash!(input));
  });
});

describe('normalizePath', () => {
  it('should return an absolute path with forward slashes only', () => {
    const resolved = normalizePath('foo/bar/baz.ts');
    expect(resolved.includes('\\')).toBe(false);
    expect(path.isAbsolute(resolved)).toBe(true);
  });

  it('should normalize Windows-style backslashes to forward slashes', () => {
    // Use posix.resolve to construct a deterministic absolute path, then convert
    // separators so the input mimics what path.resolve would produce on Windows.
    const posixAbs = path.posix.resolve('/some/project/src/foo.ts');
    const winStyle = posixAbs.replace(/\//g, '\\');
    // Round-trip through normalizePath: even if the OS-native path.resolve
    // returns backslashes, normalizePath must produce forward slashes.
    const resolved = normalizePath(winStyle);
    expect(resolved.includes('\\')).toBe(false);
  });

  it('should produce keys consistent with TypeScript SourceFile.fileName on the same input', () => {
    // ts.createSourceFile stores fileName as-is, but when TypeScript emits a
    // path it always uses forward slashes (see ts.normalizePath). Our
    // normalizePath must produce the same forward-slash representation so that
    // Map/Set lookups keyed by normalizePath(...) match SourceFile.fileName.
    const input = 'src/foo.ts';
    const resolved = normalizePath(input);
    const sf = ts.createSourceFile(resolved, '', ts.ScriptTarget.ES2021);
    expect(sf.fileName).toBe(resolved);
    expect(sf.fileName.includes('\\')).toBe(false);
  });

  it('should leave INTERNAL_PREFIX paths unchanged', () => {
    const internal = `${INTERNAL_PREFIX}/lib.foo.d.ts`;
    expect(normalizePath(internal)).toBe(internal);
  });
});
