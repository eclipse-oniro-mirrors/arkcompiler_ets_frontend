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

import * as path from 'node:path';

import * as common from '@interop-toolkits/common';
import type { Context } from '../../src/context';
import { NodeType } from '../../src/resolver/graph';
import { StaticResolver } from '../../src/resolver/static';

type ResolverUnderTest = {
  classify(
    key: string,
    dependencies: Readonly<Record<string, { path: string; sourceFilePath?: string }>>,
  ): { filePath: string; type: NodeType; isSentinel: boolean } | undefined;
};

describe('StaticResolver.classify', () => {
  const staticFile = path.resolve('/project/static.ets');
  const dynamicFile = path.resolve('/project/dynamic.ts');
  const dynamicInteropSdkFile = path.resolve('/sdk/dynamic-interop/@ohos.example.d.ets');
  const dynamicSdkFile = path.resolve('/sdk/dynamic/@ohos.example.d.ts');
  let resolver: StaticResolver;

  beforeEach(() => {
    resolver = new StaticResolver('/tools/dep_analyzer', '/cache/arktsconfig.json');
    resolver.setContext({
      fileManager: {
        staticFiles: new Set([staticFile]),
        dynamicFiles: new Set([dynamicFile]),
        isDynamicInteropSdkFile: (filePath: string) => filePath === dynamicInteropSdkFile,
        queryDynamicSdkPathForFile: () => undefined,
      } as unknown as common.fileManager.FileManager,
      cachePath: '/cache',
    } satisfies Context);
  });

  it('returns static files using their normalized native path', () => {
    expect(classify(staticFile, {})).toEqual({
      filePath: staticFile,
      type: NodeType.STATIC,
      isSentinel: false,
    });
  });

  it('maps a package-relative dependency key to its dynamic source file', () => {
    expect(
      classify('entry/src/dynamic', {
        'entry/src/dynamic': { path: '/decl/dynamic.d.ets', sourceFilePath: dynamicFile },
      }),
    ).toEqual({
      filePath: dynamicFile,
      type: NodeType.DYNAMIC,
      isSentinel: true,
    });
  });

  it('maps a dependency without a source file to a dynamic interop SDK declaration', () => {
    expect(classify('dynamic/api', { 'dynamic/api': { path: dynamicInteropSdkFile } })).toEqual({
      filePath: dynamicInteropSdkFile,
      type: NodeType.DYNAMIC,
      isSentinel: true,
    });
  });

  it('relocates a dynamic interop SDK declaration to the same-named dynamic SDK sentinel', () => {
    resolver.setContext({
      fileManager: {
        staticFiles: new Set([staticFile]),
        dynamicFiles: new Set([dynamicFile, dynamicSdkFile]),
        isDynamicInteropSdkFile: (filePath: string) => filePath === dynamicInteropSdkFile,
        queryDynamicSdkPathForFile: (filePath: string) =>
          filePath === dynamicInteropSdkFile ? dynamicSdkFile : undefined,
      } as unknown as common.fileManager.FileManager,
      cachePath: '/cache',
    } satisfies Context);

    expect(classify('dynamic/api', { 'dynamic/api': { path: dynamicInteropSdkFile } })).toEqual({
      filePath: dynamicSdkFile,
      type: NodeType.DYNAMIC,
      isSentinel: true,
    });
  });

  it('ignores dependency entries outside dynamic sources and interop SDKs', () => {
    expect(
      classify('entry/src/static', { 'entry/src/static': { path: '/decl/static.d.ets', sourceFilePath: staticFile } }),
    ).toBeUndefined();
    expect(classify('external/api', { 'external/api': { path: '/sdk/external.d.ets' } })).toBeUndefined();
  });

  function classify(
    key: string,
    dependencies: Readonly<Record<string, { path: string; sourceFilePath?: string }>>,
  ): { filePath: string; type: NodeType; isSentinel: boolean } | undefined {
    return (resolver as unknown as ResolverUnderTest).classify(key, dependencies);
  }
});
