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

import type { BuildConfig, ModuleConfig } from '../../src/contracts';
import { dependencyModulesOf, reachableModulesOf, type ModuleInfo } from '../../src/stages/configuration';
import { runConfiguration } from '../helpers/runConfiguration';

/** A main-module descriptor entry (found by `packageName` inside `dependentModuleList`). */
function entryModule(overrides: Partial<ModuleConfig> = {}): ModuleConfig {
  return {
    packageName: 'entry',
    moduleName: 'entry',
    moduleType: 'entry',
    modulePath: '/workspace/project/entry',
    sourceRoots: ['src/main'],
    entryFile: 'Index.ets',
    interopConfigPath: '',
    ...overrides,
  } as ModuleConfig;
}

/** A minimal valid slim BuildConfig; tests spread overrides over it. */
function baseBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
  return {
    plugins: [],
    buildMode: 'Debug',
    buildType: 'BUILD',
    projectRootPath: '/workspace/project',
    cachePath: 'cache',
    compileSdkVersion: 10,
    compatibleSdkVersion: 10,
    bundleName: 'com.example.app',
    moduleType: 'entry',
    moduleName: 'entry',
    packageName: 'entry',
    buildSdkPath: 'sdk/build',
    dependentModuleList: [entryModule()],
    hasMainModule: true,
    modulePath: '/workspace/project/entry',
    externalApiPaths: [],
    byteCodeHar: false,
    interopApiPaths: [],
    declgenBridgeConfigPath: '/workspace/project/declgen-bridge.json',
    interopConfigPath: '',
    ...overrides,
  } as BuildConfig;
}

describe('module dependency table', () => {
  it('accepts a valid build configuration', async () => {
    await expect(runConfiguration(baseBuildConfig())).resolves.toBeDefined();
  });

  it('accepts undefined project and module types', async () => {
    const moduleWithoutType = { ...entryModule() };
    delete moduleWithoutType.moduleType;
    const buildConfigWithoutType = {
      ...baseBuildConfig({
        dependentModuleList: [moduleWithoutType],
      }),
    };
    delete buildConfigWithoutType.moduleType;

    const { moduleTable } = await runConfiguration(buildConfigWithoutType);

    expect(moduleTable.mainModule.moduleType).toBeUndefined();
  });

  it('resolves the main module from dependentModuleList and normalizes its source roots', async () => {
    const { moduleTable: table } = await runConfiguration(
      baseBuildConfig({
        dependentModuleList: [
          entryModule({
            sourceRoots: ['src/main'],
          }),
        ],
      }),
    );

    const main = table.byPackage.get('entry');
    expect(main).toBeDefined();
    expect(main?.sourceRoots).toEqual(['/workspace/project/entry/src/main']);
    expect(table.mainModule.packageName).toBe(main?.packageName);
    expect(table.mainModule.projectRootPath).toBe('/workspace/project');
    expect(table.mainModule.cachePath).toBe('/workspace/project/cache');
    expect(table.mainModule.interopConfigPath).toBeUndefined();
  });

  it('rejects a project root that would require an implicit cwd', async () => {
    await expect(
      runConfiguration(
        baseBuildConfig({
          projectRootPath: 'relative/project',
        }),
      ),
    ).rejects.toThrow('Build configuration field "projectRootPath" must be an absolute path.');
  });

  it('rejects an empty declgen bridge config output path', async () => {
    await expect(
      runConfiguration(
        baseBuildConfig({
          declgenBridgeConfigPath: '',
        }),
      ),
    ).rejects.toThrow('Build configuration field "declgenBridgeConfigPath" must be a non-empty string.');
  });

  it('keeps the module table stable across unrelated input changes', async () => {
    const first = (await runConfiguration(baseBuildConfig())).moduleTable;
    const second = (await runConfiguration(baseBuildConfig({ compileSdkVersion: 11 }))).moduleTable;

    expect(second.modules.map((m) => m.packageName)).toEqual(first.modules.map((m) => m.packageName));
  });

  it('resolves the main module via its descriptor and materializes dependency edges', async () => {
    const { moduleTable: table } = await runConfiguration(
      baseBuildConfig({
        dependentModuleList: [
          {
            packageName: 'entry',
            moduleName: 'entry-target',
            moduleType: 'feature',
            modulePath: '/workspace/project/entry',
            sourceRoots: ['src/main'],
            entryFile: 'Index.ets',
            dependencies: ['library'],
            interopConfigPath: '',
          },
          {
            packageName: 'library',
            moduleType: 'har',
            modulePath: '/workspace/project/library',
            sourceRoots: ['src'],
            entryFile: 'Index.ets',
            dependencies: ['transitive-alias'],
            originalPackageNameMap: { 'transitive-alias': 'transitive' },
            interopConfigPath: '',
          },
          {
            packageName: 'transitive',
            moduleType: 'har',
            modulePath: '/workspace/project/transitive',
            sourceRoots: ['src'],
            entryFile: 'Index.ets',
            dependencies: [],
            interopConfigPath: '',
          },
        ],
      }),
    );

    expect(table.mainModule.moduleName).toBe('entry-target');
    expect(table.mainModule.moduleType).toBe('feature');
    expect(table.modules.map((m) => m.packageName)).toEqual(['entry', 'library', 'transitive']);

    // alias 'transitive-alias' is resolved to real name 'transitive' during normalization.
    const library = table.byPackage.get('library') as ModuleInfo;
    expect(library.dependencies).toEqual(['transitive']);
    // dependencyModulesOf materializes the edge to the actual ModuleInfo.
    expect(dependencyModulesOf(table, library).map((m) => m.packageName)).toEqual(['transitive']);
    expect(dependencyModulesOf(table, table.mainModule).map((m) => m.packageName)).toEqual(['library']);
    expect(reachableModulesOf(table).map((m) => m.packageName)).toEqual(['entry', 'library', 'transitive']);
    expect(reachableModulesOf(table, library).map((m) => m.packageName)).toEqual(['library', 'transitive']);
  });

  it('rejects dependency references that are not part of the module table', async () => {
    const buildConfig = baseBuildConfig({
      dependentModuleList: [
        entryModule(),
        {
          packageName: 'library',
          moduleType: 'har',
          modulePath: '/workspace/project/library',
          sourceRoots: ['src'],
          entryFile: 'Index.ets',
          dependencies: ['missing-package'],
          interopConfigPath: '',
        },
      ],
    });

    await expect(runConfiguration(buildConfig)).rejects.toThrow(
      'Package "library" declares unknown dependency "missing-package".',
    );
  });

  it('rejects cyclic module dependencies', async () => {
    const buildConfig = baseBuildConfig({
      dependentModuleList: [
        entryModule({
          dependencies: ['library'],
        }),
        {
          packageName: 'library',
          moduleType: 'har',
          modulePath: '/workspace/project/library',
          sourceRoots: ['src'],
          entryFile: 'Index.ets',
          dependencies: ['entry'],
          interopConfigPath: '',
        },
      ],
    });

    await expect(runConfiguration(buildConfig)).rejects.toThrow(
      'Package dependency cycle detected: entry -> library -> entry.',
    );
  });
});
