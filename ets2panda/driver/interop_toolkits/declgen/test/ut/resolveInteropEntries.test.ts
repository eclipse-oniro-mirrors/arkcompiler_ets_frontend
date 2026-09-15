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

import type { BuildConfig } from '../../src/buildConfig';
import type { Context } from '../../src/runner/context';
import { createResolveInteropEntriesStage } from '../../src/runner/stages/resolveInteropEntries';
import type { InteropEntryFiles } from '../../src/runner/stages/stageArtifacts';

type ResolveInteropEntriesScope = Parameters<ReturnType<typeof createResolveInteropEntriesStage>['run']>[0];

const PACKAGE_NAME = 'test-package';

describe('resolve-interop-entries stage', () => {
  let projectRootPath: string;
  let modulePath: string;
  let configPath: string;

  beforeEach(() => {
    projectRootPath = fs.mkdtempSync(path.join(os.tmpdir(), 'declgen-resolve-interop-entries-'));
    modulePath = path.join(projectRootPath, 'entry');
    configPath = path.join(modulePath, 'interop-config.json5');
  });

  afterEach(() => {
    fs.rmSync(projectRootPath, { recursive: true, force: true });
  });

  it('rejects a dynamic source file declared as a static interop entry', async () => {
    createSourceFile('src/dyn.ets', '');
    const context = createContext(`{ interopEntries: { static: ['src/dyn.ets'] } }`, {
      dynamicFiles: ['src/dyn.ets'],
    });

    await expect(runStage(context)).rejects.toMatchObject({
      errorMessage: {
        cause: 'src/dyn.ets is a dynamic source file, but is configured as a static interop entry.',
        position: configPath,
      },
    });
  });

  it('resolves entries whose files match the build configuration classification', async () => {
    createSourceFile('src/stat.ets', '');
    createSourceFile('src/runtime.ts', '');
    const context = createContext(`{ interopEntries: { static: ['src/stat.ets'], dynamic: ['src/runtime.ts'] } }`, {
      staticFiles: ['src/stat.ets'],
      dynamicFiles: ['src/runtime.ts'],
    });

    const entryFiles = await runStage(context);

    expect([...entryFiles.staticEntryFiles]).toEqual([normalized('src/stat.ets')]);
    expect([...entryFiles.dynamicEntryFiles]).toEqual([normalized('src/runtime.ts')]);
  });

  it('falls back to the source code directive for files outside the compile lists', async () => {
    createSourceFile('src/fallback.ets', '"use static";\n');
    const context = createContext(`{ interopEntries: { static: ['src/fallback.ets'] } }`, {});

    const entryFiles = await runStage(context);

    expect([...entryFiles.staticEntryFiles]).toEqual([normalized('src/fallback.ets')]);
  });

  function runStage(context: Context): Promise<InteropEntryFiles> {
    const scope = {
      context,
      get: (artifact: { readonly name: string }): unknown => {
        throw new Error(`required pipeline artifact is unavailable: ${artifact.name}`);
      },
    } as unknown as ResolveInteropEntriesScope;
    return createResolveInteropEntriesStage().run(scope);
  }

  function createSourceFile(relativePath: string, content: string): string {
    const filePath = path.join(modulePath, relativePath);
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, content, 'utf8');
    return filePath;
  }

  function normalized(relativePath: string): string {
    return common.fileUtils.normalizePath(path.join(modulePath, relativePath));
  }

  function createContext(
    configContent: string,
    files: { readonly staticFiles?: readonly string[]; readonly dynamicFiles?: readonly string[] },
  ): Context {
    fs.mkdirSync(modulePath, { recursive: true });
    fs.writeFileSync(configPath, configContent, 'utf8');
    const fileManager = new common.fileManager.FileManagerBuilder()
      .addModuleList([
        {
          packageName: PACKAGE_NAME,
          modulePath: common.fileUtils.normalizePath(modulePath),
          staticFiles: (files.staticFiles ?? []).map(normalized),
          dynamicFiles: (files.dynamicFiles ?? []).map(normalized),
        },
      ])
      .build();
    return {
      buildConfig: createBuildConfig(),
      fileManager,
      tsconfigPath: '',
      arktsconfigPath: '',
    };
  }

  function createBuildConfig(): BuildConfig {
    return {
      plugins: {},
      buildSdkPath: '',
      buildDynamicSdkPath: '',
      dynamicPlugins: {},
      buildMode: 'Debug',
      buildType: 'BUILD',
      projectRootPath,
      cachePath: path.join(projectRootPath, 'cache'),
      compileSdkVersion: 1,
      compatibleSdkVersion: 1,
      bundleName: 'bundle',
      moduleName: 'entry',
      packageName: PACKAGE_NAME,
      dependentModuleList: [
        {
          packageName: PACKAGE_NAME,
          modulePath,
          sourceRoots: [modulePath],
          entryFile: '',
          staticFiles: [],
          dynamicFiles: [],
          interopConfigPath: 'interop-config.json5',
        },
      ],
      sdkAliasMap: {},
      hasMainModule: true,
      modulePath,
      byteCodeHar: false,
      declgenBridgeConfigPath: '',
      interopConfigPath: '',
      externalApiPaths: [],
      pandaSdkPath: '',
      sdkPaths: {
        staticSdkPaths: [],
        dynamicSdkPaths: [],
        staticInteropSdkPaths: [],
        dynamicInteropSdkPaths: [],
      },
    };
  }
});
