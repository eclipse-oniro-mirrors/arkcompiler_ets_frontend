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

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';
import * as ts from 'typescript';
import * as common from '@interop-toolkits/common';
import type { Context } from '../../src/context';
import { DynamicResolver } from '../../src/resolver/dynamic';
import { NodeType } from '../../src/resolver/graph';

type ResolverUnderTest = {
  resolveSpecifier(specifier: string, containingFile: string): string | undefined;
  resolveModuleName(specifier: string, containingFile: string): ts.ResolvedModuleWithFailedLookupLocations;
};

describe('DynamicResolver.resolveSpecifier', () => {
  let projectRoot: string;
  let containingFile: string;
  let dynamicSdkPaths: Map<string, string>;
  let staticInteropSdkPaths: Map<string, string>;
  let staticSdkPaths: Map<string, string>;
  let resolver: DynamicResolver;

  beforeEach(() => {
    projectRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'dynamic-resolver-'));
    containingFile = createFile('src/entry.ts', 'export {};');
    dynamicSdkPaths = new Map<string, string>();
    staticInteropSdkPaths = new Map<string, string>();
    staticSdkPaths = new Map<string, string>();
    resolver = new DynamicResolver(projectRoot, {
      allowJs: true,
      module: ts.ModuleKind.CommonJS,
      moduleResolution: ts.ModuleResolutionKind.NodeJs,
      target: ts.ScriptTarget.ES2021,
    });
    resolver.setContext({
      fileManager: {
        queryDynamicSdkPath: (specifier: string) => dynamicSdkPaths.get(specifier),
        queryStaticInteropSdkPath: (specifier: string) => staticInteropSdkPaths.get(specifier),
        queryStaticSdkPath: (specifier: string) => staticSdkPaths.get(specifier),
      } as unknown as common.fileManager.FileManager,
      cachePath: projectRoot,
    } satisfies Context);
  });

  afterEach(() => {
    fs.rmSync(projectRoot, { recursive: true, force: true });
  });

  it('uses default TypeScript resolution for an extensionless module', () => {
    const dependency = createFile('src/dependency.ts', 'export const value = 1;');

    expect(resolveSpecifier('./dependency')).toBe(dependency);
  });

  it('maps a JavaScript resolution to an adjacent .d.ets declaration', () => {
    createFile('src/generated.js', 'module.exports = {};');
    const declaration = createFile('src/generated.d.ets', 'export declare const value: number;');

    expect(resolveSpecifier('./generated.js')).toBe(declaration);
  });

  it('resolves dynamic SDK module names through FileManager', () => {
    const declaration = createFile('sdk/dynamic/@ohos.example.d.ts', 'export declare const value: number;');
    dynamicSdkPaths.set('@ohos.example', declaration);

    expect(resolveSpecifier('@ohos.example')).toBe(declaration);
  });

  it('resolves static interop SDK module names through FileManager', () => {
    const declaration = createFile('sdk/static-interop/@ohos.example.d.ts', 'export declare const value: number;');
    staticInteropSdkPaths.set('@ohos.example', declaration);

    expect(resolveSpecifier('static@ohos.example')).toBe(declaration);
    expect(resolveSpecifier('STATIC@ohos.example')).toBe(declaration);
  });

  it('relocates a static interop SDK resolution to the same-named static SDK', () => {
    const interopDeclaration = createFile('sdk/static-interop/@ohos.example.d.ts', 'export declare const a: number;');
    const staticDeclaration = createFile('sdk/static/@ohos.example.d.ets', 'export declare const a: number;');
    staticInteropSdkPaths.set('@ohos.example', interopDeclaration);
    staticSdkPaths.set('@ohos.example', staticDeclaration);

    expect(resolveSpecifier('static@ohos.example')).toBe(staticDeclaration);
  });

  it.each([
    ['./explicit.ts', 'src/explicit.ts'],
    ['./explicit.ets', 'src/explicit.ets'],
    ['./explicit.d.ets', 'src/explicit.d.ets'],
  ])('resolves an explicit source extension: %s', (specifier, relativePath) => {
    const dependency = createFile(relativePath, 'export {};');

    expect(resolveSpecifier(specifier)).toBe(dependency);
  });

  it('returns undefined when no strategy resolves the specifier', () => {
    expect(resolveSpecifier('./missing')).toBeUndefined();
  });

  it.each([
    [{ entry: [] }, 'Invalid SDK alias map entry for package "entry": expected an object.'],
    [
      { entry: { alias: { originalAPIName: '@ohos.example', isStatic: 'true' } } },
      'Invalid SDK alias map entry "entry.alias": expected { originalAPIName: string, isStatic: boolean }.',
    ],
    [
      { entry: { alias: { originalAPIName: 1, isStatic: true } } },
      'Invalid SDK alias map entry "entry.alias": expected { originalAPIName: string, isStatic: boolean }.',
    ],
  ])('rejects an invalid SDK alias map', (sdkAliasMap, message) => {
    expect(() => createResolver(sdkAliasMap)).toThrow(message);
  });

  it('caches resolved and unresolved specifiers for the same importer', () => {
    const dependency = createFile('src/dependency.ts', 'export const value = 1;');
    const resolveModuleName = jest.spyOn(resolver as unknown as ResolverUnderTest, 'resolveModuleName');

    expect(resolveSpecifier('./dependency')).toBe(dependency);
    expect(resolveSpecifier('./dependency')).toBe(dependency);
    expect(resolveSpecifier('./missing')).toBeUndefined();
    expect(resolveSpecifier('./missing')).toBeUndefined();

    expect(resolveModuleName).toHaveBeenCalledTimes(2);
  });

  it('keeps relative specifier cache entries isolated by importer', () => {
    const firstImporter = createFile('first/entry.ts', 'export {};');
    const secondImporter = createFile('second/entry.ts', 'export {};');
    const firstDependency = createFile('first/dependency.ts', 'export const first = 1;');
    const secondDependency = createFile('second/dependency.ts', 'export const second = 2;');
    const resolveModuleName = jest.spyOn(resolver as unknown as ResolverUnderTest, 'resolveModuleName');

    expect(resolveSpecifier('./dependency', firstImporter)).toBe(firstDependency);
    expect(resolveSpecifier('./dependency', secondImporter)).toBe(secondDependency);
    expect(resolveSpecifier('./dependency', firstImporter)).toBe(firstDependency);

    expect(resolveModuleName).toHaveBeenCalledTimes(2);
  });

  it('recursively resolves dynamic dependencies outside the entry files', () => {
    const entry = createFile('src/entry.ts', "import './intermediate';");
    const intermediate = createFile('src/intermediate.ts', "import './leaf';");
    const leaf = createFile('src/leaf.ts', 'export const leaf = true;');
    resolver.setContext({
      fileManager: {
        dynamicFiles: new Set([entry, intermediate, leaf]),
        staticFiles: new Set(),
        isStaticInteropSdkFile: () => false,
      } as unknown as common.fileManager.FileManager,
      cachePath: projectRoot,
    } satisfies Context);

    const result = resolver.resolve([entry]);

    expect(result.nodes.get(entry)?.dependencies).toEqual([intermediate]);
    expect(result.nodes.get(intermediate)?.dependencies).toEqual([leaf]);
    expect(result.nodes.get(leaf)?.dependencies).toEqual([]);
  });

  it('terminates recursive resolution for circular dynamic dependencies', () => {
    const first = createFile('src/first.ts', "import './second';");
    const second = createFile('src/second.ts', "import './first';");
    resolver.setContext({
      fileManager: {
        dynamicFiles: new Set([first, second]),
        staticFiles: new Set(),
        isStaticInteropSdkFile: () => false,
      } as unknown as common.fileManager.FileManager,
      cachePath: projectRoot,
    } satisfies Context);

    const result = resolver.resolve([first]);

    expect(result.nodes.get(first)?.dependencies).toEqual([second]);
    expect(result.nodes.get(second)?.dependencies).toEqual([first]);
  });

  it('relocates a static interop SDK dependency to the same-named static SDK sentinel', () => {
    const entry = createFile('src/entry.ts', "import 'static@ohos.example';");
    const interopSdkFile = createFile('sdk/static-interop/@ohos.example.d.ts', 'export declare const value: number;');
    const staticSdkFile = createFile('sdk/static/@ohos.example.d.ets', 'export declare const value: number;');
    resolver.setContext({
      fileManager: {
        queryStaticInteropSdkPath: (specifier: string) => (specifier === '@ohos.example' ? interopSdkFile : undefined),
        queryStaticSdkPath: (specifier: string) => (specifier === '@ohos.example' ? staticSdkFile : undefined),
        dynamicFiles: new Set([entry]),
        staticFiles: new Set([staticSdkFile]),
        isStaticInteropSdkFile: () => false,
      } as unknown as common.fileManager.FileManager,
      cachePath: projectRoot,
    } satisfies Context);

    const result = resolver.resolve([entry]);

    expect(result.nodes.get(entry)?.dependencies).toEqual([staticSdkFile]);
    expect(result.nodes.get(staticSdkFile)).toMatchObject({ type: NodeType.STATIC, isSentinel: true });
    expect(result.sentinels).toEqual([staticSdkFile]);
    expect(result.nodes.has(interopSdkFile)).toBe(false);
  });

  it('keeps the static interop SDK file when the static SDK has no counterpart', () => {
    const entry = createFile('src/entry.ts', "import 'static@ohos.example';");
    const interopSdkFile = createFile('sdk/static-interop/@ohos.example.d.ts', 'export declare const value: number;');
    resolver.setContext({
      fileManager: {
        queryStaticInteropSdkPath: (specifier: string) => (specifier === '@ohos.example' ? interopSdkFile : undefined),
        queryStaticSdkPath: () => undefined,
        dynamicFiles: new Set([entry]),
        staticFiles: new Set(),
        isStaticInteropSdkFile: (filePath: string) => filePath === interopSdkFile,
      } as unknown as common.fileManager.FileManager,
      cachePath: projectRoot,
    } satisfies Context);

    const result = resolver.resolve([entry]);

    expect(result.nodes.get(entry)?.dependencies).toEqual([interopSdkFile]);
    expect(result.nodes.get(interopSdkFile)).toMatchObject({ type: NodeType.STATIC, isSentinel: true });
    expect(result.sentinels).toEqual([interopSdkFile]);
  });

  function resolveSpecifier(specifier: string, importer = containingFile): string | undefined {
    return (resolver as unknown as ResolverUnderTest).resolveSpecifier(specifier, importer);
  }

  function createResolver(sdkAliasMap: unknown): DynamicResolver {
    return new DynamicResolver(
      projectRoot,
      {
        allowJs: true,
        module: ts.ModuleKind.CommonJS,
        moduleResolution: ts.ModuleResolutionKind.NodeJs,
        target: ts.ScriptTarget.ES2021,
      },
      sdkAliasMap as never,
    );
  }

  function createFile(relativePath: string, content: string): string {
    const filePath = path.join(projectRoot, relativePath);
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, content, 'utf-8');
    return filePath;
  }
});
