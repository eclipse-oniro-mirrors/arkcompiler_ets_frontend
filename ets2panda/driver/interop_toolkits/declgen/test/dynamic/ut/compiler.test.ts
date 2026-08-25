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
import { Compiler, defaultCompilerOptions } from '../../../src/dynamic/compiler/compiler';

function createTempDir(prefix: string): string {
  return fs.mkdtempSync(path.join(os.tmpdir(), prefix));
}

function writeFile(filePath: string, content: string): void {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, content, 'utf8');
}

function createCompilerOptions(outDir: string, overrides?: ts.CompilerOptions): ts.CompilerOptions {
  return {
    ...defaultCompilerOptions(),
    outDir,
    declaration: true,
    emitDeclarationOnly: true,
    noEmit: false,
    ...overrides,
  };
}

describe('Compiler', () => {
  jest.setTimeout(10000);

  it('should compile in non-incremental mode and expose program/typeChecker', () => {
    const tempDir = createTempDir('compiler-non-incremental-');
    const entryFile = path.join(tempDir, 'entry.ts');
    const outDir = path.join(tempDir, 'out');

    writeFile(entryFile, 'export const value: number = 1;');

    const compiler = new Compiler([entryFile], [], createCompilerOptions(outDir));
    compiler.compile();

    const sourceFile = compiler.getSourceFile(entryFile);
    expect(sourceFile).toBeTruthy();
    expect(compiler.program).toBeTruthy();
    expect(compiler.typeChecker).toBeTruthy();

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should persist tsbuildinfo in incremental mode after emit', () => {
    const tempDir = createTempDir('compiler-incremental-buildinfo-');
    const entryFile = path.join(tempDir, 'entry.ts');
    const outDir = path.join(tempDir, 'out');
    const tsBuildInfoFile = path.join(tempDir, '.tsbuildinfo');

    writeFile(entryFile, 'export const value: number = 1;');

    const compiler = new Compiler(
      [entryFile],
      [],
      createCompilerOptions(outDir, {
        incremental: true,
        tsBuildInfoFile,
      }),
    );

    compiler.compile();
    compiler.emit(() => {
      return;
    });

    expect(fs.existsSync(tsBuildInfoFile)).toBe(true);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should treat all files as affected on first incremental compile, then track affected set on second compile', () => {
    const tempDir = createTempDir('compiler-incremental-affected-');
    const entryFile = path.join(tempDir, 'entry.ts');
    const outDir = path.join(tempDir, 'out');
    const tsBuildInfoFile = path.join(tempDir, '.tsbuildinfo');

    writeFile(entryFile, 'export const value: number = 1;');

    const compiler = new Compiler(
      [entryFile],
      [],
      createCompilerOptions(outDir, {
        incremental: true,
        tsBuildInfoFile,
      }),
    );

    compiler.compile();
    expect(compiler.isFileAffected(entryFile)).toBe(true);

    compiler.emit(() => {
      return;
    });
    compiler.compile();

    expect(compiler.isFileAffected(entryFile)).toBe(false);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should mark downstream file as affected after dependency file changes', () => {
    const tempDir = createTempDir('compiler-incremental-dependency-');
    const baseFile = path.join(tempDir, 'base.ts');
    const consumerFile = path.join(tempDir, 'consumer.ts');
    const outDir = path.join(tempDir, 'out');
    const tsBuildInfoFile = path.join(tempDir, '.tsbuildinfo');

    writeFile(baseFile, 'export interface Model { value: number; }');
    writeFile(consumerFile, 'import { Model } from "./base"; export const data: Model = { value: 1 };');

    const options = createCompilerOptions(outDir, {
      incremental: true,
      tsBuildInfoFile,
    });

    const initialCompiler = new Compiler([baseFile, consumerFile], [], options);
    initialCompiler.compile();
    initialCompiler.emit(() => {
      return;
    });

    writeFile(baseFile, 'export interface Model { value: number; flag?: boolean; }');

    const incrementalCompiler = new Compiler([baseFile, consumerFile], [], options);
    incrementalCompiler.compile();

    expect(incrementalCompiler.isFileAffected(baseFile)).toBe(true);
    expect(incrementalCompiler.isFileAffected(consumerFile)).toBe(true);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  it('should refresh program with updated source files from cache', () => {
    const tempDir = createTempDir('compiler-refresh-');
    const entryFile = path.join(tempDir, 'entry.ts');
    const outDir = path.join(tempDir, 'out');

    writeFile(entryFile, 'export const oldName = 1;');

    const compiler = new Compiler([entryFile], [], createCompilerOptions(outDir));
    compiler.compile();

    const updatedSource = ts.createSourceFile(
      entryFile,
      'export const newName = 2;',
      ts.ScriptTarget.Latest,
      true,
      ts.ScriptKind.TS,
    );

    compiler.updateFile(entryFile, updatedSource);
    compiler.refreshProgram();

    const refreshed = compiler.getSourceFile(entryFile);
    expect(refreshed).toBeTruthy();
    expect(refreshed?.text.includes('newName')).toBe(true);

    fs.rmSync(tempDir, { recursive: true, force: true });
  });

  describe('emit onEmitted hook', () => {
    it('invokes onEmitted with the default-writer path and final content', () => {
      const tempDir = createTempDir('compiler-emit-default-');
      const entryFile = path.join(tempDir, 'entry.ts');
      const outDir = path.join(tempDir, 'out');

      writeFile(entryFile, 'export const value: number = 1;');

      const compiler = new Compiler([entryFile], [], createCompilerOptions(outDir));
      compiler.compile();

      const calls: Array<{ absSrc: string; path: string; finalContent: string }> = [];
      compiler.emit(undefined, (absSrc, info) => {
        calls.push({ absSrc, ...info });
      });

      expect(calls.length).toBe(1);
      const call = calls[0]!;
      expect(call.absSrc).toBe(entryFile);
      expect(call.path.startsWith(outDir)).toBeTruthy();
      expect(fs.existsSync(call.path)).toBe(true);
      expect(fs.readFileSync(call.path, 'utf8')).toBe(call.finalContent);

      fs.rmSync(tempDir, { recursive: true, force: true });
    });

    it('skips onEmitted when a custom writeFile returns void', () => {
      const tempDir = createTempDir('compiler-emit-custom-void-');
      const entryFile = path.join(tempDir, 'entry.ts');
      const outDir = path.join(tempDir, 'out');

      writeFile(entryFile, 'export const value: number = 1;');

      const compiler = new Compiler([entryFile], [], createCompilerOptions(outDir));
      compiler.compile();

      let onEmittedCalls = 0;
      compiler.emit(
        () => {
          return;
        },
        () => {
          onEmittedCalls++;
        },
      );

      expect(onEmittedCalls).toBe(0);
      fs.rmSync(tempDir, { recursive: true, force: true });
    });

    it('invokes onEmitted when a custom writeFile opts in via { artifactPath, finalContent }', () => {
      const tempDir = createTempDir('compiler-emit-custom-optin-');
      const entryFile = path.join(tempDir, 'entry.ts');
      const outDir = path.join(tempDir, 'out');

      writeFile(entryFile, 'export const value: number = 1;');

      const compiler = new Compiler([entryFile], [], createCompilerOptions(outDir));
      compiler.compile();

      const calls: Array<{ absSrc: string; path: string; finalContent: string }> = [];
      compiler.emit(
        () => {
          return { artifactPath: '/virtual/foo.d.ets', finalContent: '<<final>>' };
        },
        (absSrc, info) => {
          calls.push({ absSrc, ...info });
        },
      );

      expect(calls.length).toBe(1);
      const call = calls[0]!;
      expect(call.absSrc).toBe(entryFile);
      expect(call.path).toBe('/virtual/foo.d.ets');
      expect(call.finalContent).toBe('<<final>>');

      fs.rmSync(tempDir, { recursive: true, force: true });
    });
  });
});
