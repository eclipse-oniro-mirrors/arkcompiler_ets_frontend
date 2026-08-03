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

import type { BuildConfig, ModuleConfig } from '../../src/contracts';
import type { ILogger } from '../../src/logger';
import { Pipeline } from '../../src/pipeline';
import type { GlueGenContext } from '../../src/pipeline/context';
import { createArkTSConfigStage } from '../../src/stages/arktsconfig';
import type { ArkTSConfig } from '../../src/stages/arktsconfig/arktsconfig';
import { createConfigurationStage } from '../../src/stages/configuration';

const SILENT_LOGGER: ILogger = {
  printInfo: () => undefined,
  printWarn: () => undefined,
  printDebug: () => undefined,
  printError: () => undefined,
  printErrorAndExit: () => undefined,
};

describe('arktsconfig stage', () => {
  let projectRootPath: string;

  beforeEach(async () => {
    projectRootPath = await fs.mkdtemp(path.join(os.tmpdir(), 'gluegen-arktsconfig-'));
  });

  afterEach(async () => {
    await fs.rm(projectRootPath, { recursive: true, force: true });
  });

  it('writes the main module contract and publishes only its file path', async () => {
    const externalApiPath = path.join(projectRootPath, 'external-api');
    const interopSdkPath = path.join(projectRootPath, 'interop-sdk');
    const dynamicModulePath = path.join(projectRootPath, 'dynamic');
    await fs.mkdir(path.join(externalApiPath, 'nested'), { recursive: true });
    await fs.mkdir(path.join(externalApiPath, 'arkui', 'runtime-api', 'components'), {
      recursive: true,
    });
    await fs.mkdir(path.join(interopSdkPath, 'api', 'subdir'), { recursive: true });
    await fs.mkdir(path.join(interopSdkPath, 'api', 'arkui', 'runtime-api', 'nested'), {
      recursive: true,
    });
    await fs.mkdir(path.join(interopSdkPath, 'component'), { recursive: true });
    await fs.mkdir(path.join(interopSdkPath, 'kits'), { recursive: true });
    await Promise.all([
      fs.mkdir(path.join(projectRootPath, 'entry', 'src', 'main'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'entry', 'src', 'common'), { recursive: true }),
      fs.mkdir(path.join(dynamicModulePath, 'src', 'main'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'library', 'src'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'transitive', 'src'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'unused'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'hybrid', 'entry'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'hybrid', 'src'), { recursive: true }),
      fs.mkdir(path.join(projectRootPath, 'unspecified'), { recursive: true }),
    ]);
    await Promise.all([
      fs.writeFile(path.join(projectRootPath, 'entry', 'Index.ets'), "'use static'\n"),
      fs.writeFile(path.join(projectRootPath, 'entry', 'src', 'main', 'Main.ets'), 'export const main = true;\n'),
      fs.writeFile(
        path.join(projectRootPath, 'entry', 'src', 'common', 'Types.d.ets'),
        'export declare const value: boolean;\n',
      ),
      fs.writeFile(path.join(dynamicModulePath, 'src', 'main', 'Index.ets'), "'use static'\n"),
      fs.writeFile(path.join(projectRootPath, 'library', 'Index.ets'), 'export const dynamicLibraryEntry = true;\n'),
      fs.writeFile(path.join(projectRootPath, 'library', 'src', 'Library.ets'), 'export const library = true;\n'),
      fs.writeFile(path.join(projectRootPath, 'transitive', 'Index.ets'), "'use static'\n"),
      fs.writeFile(
        path.join(projectRootPath, 'transitive', 'src', 'Types.d.ets'),
        'export declare const transitive: boolean;\n',
      ),
      fs.writeFile(path.join(projectRootPath, 'unused', 'Index.ets'), "'use static'\n"),
      fs.writeFile(path.join(projectRootPath, 'hybrid', 'entry', 'Index.ets'), "'use static'\n"),
      fs.writeFile(
        path.join(projectRootPath, 'hybrid', 'src', 'StaticPart.ets'),
        "'use static'\nexport const staticPart = true;\n",
      ),
      fs.writeFile(
        path.join(projectRootPath, 'hybrid', 'src', 'DynamicPart.ets'),
        'export const dynamicPart = true;\n',
      ),
      fs.writeFile(
        path.join(projectRootPath, 'unspecified', 'Index.ets'),
        'export const dynamicUnspecifiedEntry = true;\n',
      ),
      fs.writeFile(path.join(externalApiPath, '@ohos.sample.d.ets'), 'export declare const sample: string;\n'),
      fs.writeFile(path.join(externalApiPath, 'nested', 'feature.d.ets'), 'export declare const feature: string;\n'),
      fs.writeFile(
        path.join(externalApiPath, 'arkui', 'runtime-api', 'components', 'Widget.d.ets'),
        'export declare class Widget {}\n',
      ),
      fs.writeFile(path.join(externalApiPath, 'ignored.ets'), 'export const ignored = true;\n'),
      fs.writeFile(path.join(interopSdkPath, 'api', '@ohos.storage.d.ets'), 'export declare const storage: string;\n'),
      fs.writeFile(path.join(interopSdkPath, 'api', '@system.router.d.ets'), 'export declare const router: string;\n'),
      fs.writeFile(
        path.join(interopSdkPath, 'api', 'subdir', 'normal.d.ets'),
        'export declare const normal: string;\n',
      ),
      fs.writeFile(
        path.join(interopSdkPath, 'api', 'arkui', 'runtime-api', 'nested', 'Runtime.d.ets'),
        'export declare const runtime: string;\n',
      ),
      fs.writeFile(path.join(interopSdkPath, 'component', 'button.d.ets'), 'export declare const button: string;\n'),
      fs.writeFile(
        path.join(interopSdkPath, 'kits', '@kit.Ignored.d.ets'),
        'export declare const ignoredKit: string;\n',
      ),
      fs.writeFile(path.join(interopSdkPath, 'api', 'ignored.ets'), 'export const ignored = true;\n'),
      fs.writeFile(
        path.join(dynamicModulePath, 'decl-fileInfo.json'),
        JSON.stringify({
          files: {
            'src/main/Index': {
              declPath: path.join(dynamicModulePath, 'Index.d.ets'),
              filePath: path.join(dynamicModulePath, 'src', 'main', 'Index.ets'),
              ohmUrl: '@normalized:N&&&dynamic/Index&',
            },
            'src/common/utils/Helper': {
              declPath: path.join(dynamicModulePath, 'Helper.d.ets'),
              filePath: path.join(dynamicModulePath, 'src', 'common', 'utils', 'Helper.ets'),
              ohmUrl: '@normalized:N&&&dynamic/utils/Helper&',
            },
            'other/Loose': {
              declPath: path.join(dynamicModulePath, 'Loose.d.ets'),
              filePath: path.join(dynamicModulePath, 'other', 'Loose.ets'),
              ohmUrl: '@normalized:N&&&dynamic/other/Loose&',
            },
          },
        }),
      ),
      fs.writeFile(
        path.join(projectRootPath, 'hybrid', 'decl-fileInfo.json'),
        JSON.stringify({
          files: {
            'src/DynamicPart': {
              declPath: path.join(projectRootPath, 'hybrid', 'DynamicPart.d.ets'),
              filePath: path.join(projectRootPath, 'hybrid', 'src', 'DynamicPart.ets'),
              ohmUrl: '@normalized:N&&&hybrid/DynamicPart&',
            },
          },
        }),
      ),
    ]);
    const buildConfig = createBuildConfig(projectRootPath);
    const context: GlueGenContext = {
      buildConfig,
      logger: SILENT_LOGGER,
      runtime: { nativeExecutablePath: '' },
    };

    const outputPath = await Pipeline.start<GlueGenContext>()
      .stage(createConfigurationStage())
      .stage(createArkTSConfigStage())
      .run(context);

    expect(outputPath).toBe(path.join(projectRootPath, 'cache', 'entry', 'arktsconfig.json'));

    const config = JSON.parse(await fs.readFile(outputPath, 'utf8')) as ArkTSConfig;
    expect(Object.keys(config)).toEqual(['compilerOptions']);
    expect(Object.keys(config.compilerOptions)).toEqual([
      'package',
      'baseUrl',
      'rootDir',
      'paths',
      'dependencies',
      'cacheDir',
      'declgenV2OutPath',
    ]);
    expect(config.compilerOptions.package).toBe('entry');
    expect(config.compilerOptions.baseUrl).toBe(path.join(projectRootPath, 'entry'));
    expect(config.compilerOptions.rootDir).toBe(projectRootPath);
    expect(config.compilerOptions.cacheDir).toBe(path.join(projectRootPath, 'cache'));
    expect(config.compilerOptions.declgenV2OutPath).toBe(config.compilerOptions.cacheDir);
    expect(config.compilerOptions.paths['@ohos.sample']).toEqual([path.join(externalApiPath, '@ohos.sample')]);
    expect(config.compilerOptions.paths['nested.feature']).toEqual([path.join(externalApiPath, 'nested', 'feature')]);
    expect(config.compilerOptions.paths.Widget).toEqual([
      path.join(externalApiPath, 'arkui', 'runtime-api', 'components', 'Widget'),
    ]);
    expect(config.compilerOptions.paths.ignored).toBeUndefined();
    expect(config.compilerOptions.paths.entry).toEqual([
      path.join(projectRootPath, 'entry', 'src', 'common'),
      path.join(projectRootPath, 'entry', 'src', 'main'),
      path.join(projectRootPath, 'entry'),
    ]);
    expect(config.compilerOptions.paths['entry/Index']).toEqual([path.join(projectRootPath, 'entry', 'Index.ets')]);
    expect(config.compilerOptions.paths['entry/src/main/Main']).toEqual([
      path.join(projectRootPath, 'entry', 'src', 'main', 'Main.ets'),
    ]);
    expect(config.compilerOptions.paths['entry/src/common/Types']).toEqual([
      path.join(projectRootPath, 'entry', 'src', 'common', 'Types.d.ets'),
    ]);
    expect(config.compilerOptions.paths.library).toEqual([
      path.join(projectRootPath, 'library', 'src'),
      path.join(projectRootPath, 'library'),
    ]);
    expect(config.compilerOptions.paths['library/src/Library']).toEqual([
      path.join(projectRootPath, 'library', 'src', 'Library.ets'),
    ]);
    expect(config.compilerOptions.paths.transitive).toEqual([
      path.join(projectRootPath, 'transitive', 'src'),
      path.join(projectRootPath, 'transitive'),
    ]);
    expect(config.compilerOptions.paths['transitive/src/Types']).toEqual([
      path.join(projectRootPath, 'transitive', 'src', 'Types.d.ets'),
    ]);
    expect(config.compilerOptions.paths.unused).toBeUndefined();
    expect(config.compilerOptions.paths.hybrid).toBeUndefined();
    expect(config.compilerOptions.paths['hybrid/Index']).toBeUndefined();
    expect(config.compilerOptions.paths['hybrid/src/StaticPart']).toEqual([
      path.join(projectRootPath, 'hybrid', 'src', 'StaticPart.ets'),
    ]);
    expect(config.compilerOptions.paths['hybrid/src/DynamicPart']).toBeUndefined();
    expect(config.compilerOptions.paths.unspecified).toBeUndefined();
    expect(config.compilerOptions.dependencies.library).toEqual({
      language: 'ets',
      path: path.join(projectRootPath, 'library', 'library.abc'),
      ohmUrl: 'library',
      mainFile: 'Index',
    });
    expect(config.compilerOptions.dependencies['library-alias']).toEqual(config.compilerOptions.dependencies.library);
    expect(config.compilerOptions.dependencies.transitive).toEqual({
      language: 'ets',
      path: path.join(projectRootPath, 'transitive', 'transitive.abc'),
      ohmUrl: 'transitive',
      mainFile: 'Index',
    });
    expect(config.compilerOptions.dependencies.unused).toBeUndefined();
    expect(config.compilerOptions.dependencies['std/core']).toEqual({
      language: 'ets',
      path: path.join(os.tmpdir(), 'gluegen-public-sdk', 'build-tools', 'ets2panda', 'lib', 'etsstdlib.abc'),
      ohmUrl: 'std/core',
    });
    const dynamicEntry = {
      language: 'js',
      path: path.join(projectRootPath, 'dynamic', 'Index.d.ets'),
      sourceFilePath: path.join(projectRootPath, 'dynamic', 'src', 'main', 'Index.ets'),
      ohmUrl: '@normalized:N&dynamic&com.example.dynamic&dynamic/Index&',
    };
    expect(config.compilerOptions.dependencies.dynamic).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/src/main/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic-alias']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic-alias/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/utils/Helper']?.path).toBe(
      path.join(projectRootPath, 'dynamic', 'Helper.d.ets'),
    );
    expect(config.compilerOptions.dependencies['dynamic/other/Loose']?.path).toBe(
      path.join(projectRootPath, 'dynamic', 'Loose.d.ets'),
    );
    expect(config.compilerOptions.dependencies['hybrid/DynamicPart']?.path).toBe(
      path.join(projectRootPath, 'hybrid', 'DynamicPart.d.ets'),
    );
    expect(config.compilerOptions.dependencies['hybrid/src/DynamicPart']?.path).toBe(
      path.join(projectRootPath, 'hybrid', 'DynamicPart.d.ets'),
    );
    expect(config.compilerOptions.dependencies.hybrid).toBeUndefined();
    expect(config.compilerOptions.dependencies['dynamic/@ohos.storage']).toEqual({
      language: 'js',
      path: path.join(interopSdkPath, 'api', '@ohos.storage.d.ets'),
      ohmUrl: '@ohos:storage',
      alias: ['@ohos.storage', 'dynamic@ohos.storage'],
    });
    expect(config.compilerOptions.dependencies['dynamic/@system.router']).toEqual({
      language: 'js',
      path: path.join(interopSdkPath, 'api', '@system.router.d.ets'),
      ohmUrl: '@native:system.router',
      alias: ['@system.router', 'dynamic@system.router'],
    });
    expect(config.compilerOptions.dependencies['dynamic/subdir/normal']).toEqual({
      language: 'js',
      path: path.join(interopSdkPath, 'api', 'subdir', 'normal.d.ets'),
      ohmUrl: '',
      alias: ['subdir/normal', 'dynamicsubdir/normal'],
    });
    expect(config.compilerOptions.dependencies['dynamic/Runtime']?.path).toBe(
      path.join(interopSdkPath, 'api', 'arkui', 'runtime-api', 'nested', 'Runtime.d.ets'),
    );
    expect(config.compilerOptions.dependencies['component/button']).toEqual({
      language: 'js',
      path: path.join(interopSdkPath, 'component', 'button.d.ets'),
      ohmUrl: '',
      alias: ['button', 'dynamicbutton'],
    });
    expect(config.compilerOptions.dependencies['dynamic/@kit.Ignored']).toBeUndefined();
    expect(config.compilerOptions.dependencies['dynamic/ignored']).toBeUndefined();
  });

  it('adds the entry module declarations when the entry module uses ArkTS 1.1', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const declFilesPath = path.join(entryModulePath, 'decl-fileInfo.json');
    const entryDeclPath = path.join(entryModulePath, 'Index.d.ets');
    await fs.mkdir(entryModulePath, { recursive: true });
    await Promise.all([
      fs.writeFile(
        declFilesPath,
        JSON.stringify({
          files: {
            Index: {
              declPath: entryDeclPath,
              filePath: path.join(entryModulePath, 'Index.ets'),
              ohmUrl: '@normalized:N&&&entry/Index&',
            },
          },
        }),
      ),
      fs.writeFile(path.join(entryModulePath, 'Index.ets'), 'export const dynamicEntry = true;\n'),
    ]);

    const baseConfig = createBuildConfig(projectRootPath);
    const buildConfig: BuildConfig = {
      ...baseConfig,
      externalApiPaths: [],
      interopApiPaths: [],
      dependentModuleList: baseConfig.dependentModuleList.map((module) =>
        module.packageName === 'entry'
          ? {
              ...module,
              language: '1.1',
              declFilesPath,
            }
          : module,
      ),
    };
    const context: GlueGenContext = {
      buildConfig,
      logger: SILENT_LOGGER,
      runtime: { nativeExecutablePath: '' },
    };

    const outputPath = await Pipeline.start<GlueGenContext>()
      .stage(createConfigurationStage())
      .stage(createArkTSConfigStage())
      .run(context);
    const config = JSON.parse(await fs.readFile(outputPath, 'utf8')) as ArkTSConfig;
    const expectedEntry = {
      language: 'js',
      path: entryDeclPath,
      sourceFilePath: path.join(entryModulePath, 'Index.ets'),
      ohmUrl: '@normalized:N&&&entry/Index&',
    };
    expect(config.compilerOptions.dependencies.entry).toEqual(expectedEntry);
    expect(config.compilerOptions.dependencies['entry/Index']).toEqual(expectedEntry);
  });

  it('lets the highest-priority sourceRoot claim a dynamic transformed key', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const declFilesPath = path.join(entryModulePath, 'decl-fileInfo.json');
    const entryFile = path.join(entryModulePath, 'src', 'target1', 'Index.ets');
    await fs.mkdir(path.dirname(entryFile), { recursive: true });
    await fs.writeFile(entryFile, 'export const entry = true;\n');

    const files = Object.fromEntries(
      ['target3', 'target2', 'target1'].map((target) => [
        `src/${target}/Foo`,
        {
          declPath: path.join(entryModulePath, `${target}.d.ets`),
          filePath: path.join(entryModulePath, 'src', target, 'Foo.ets'),
          ohmUrl: `@normalized:N&&&entry/Foo&`,
        },
      ]),
    );
    await fs.writeFile(
      declFilesPath,
      JSON.stringify({
        files: {
          ...files,
          'src/target1/Index': {
            declPath: path.join(entryModulePath, 'Index.d.ets'),
            filePath: entryFile,
            ohmUrl: '@normalized:N&&&entry/Index&',
          },
        },
      }),
    );

    const baseConfig = createBuildConfig(projectRootPath);
    const buildConfig: BuildConfig = {
      ...baseConfig,
      externalApiPaths: [],
      interopApiPaths: [],
      dependentModuleList: [
        {
          packageName: 'entry',
          moduleName: 'entry',
          moduleType: 'entry',
          modulePath: entryModulePath,
          sourceRoots: ['src/target3', 'src/target2', 'src/target1'],
          entryFile: 'src/target1/Index.ets',
          declFilesPath,
          dependencies: [],
          language: '1.1',
        },
      ],
    };
    const outputPath = await Pipeline.start<GlueGenContext>()
      .stage(createConfigurationStage())
      .stage(createArkTSConfigStage())
      .run({
        buildConfig,
        logger: SILENT_LOGGER,
        runtime: { nativeExecutablePath: '' },
      });
    const config = JSON.parse(await fs.readFile(outputPath, 'utf8')) as ArkTSConfig;

    expect(config.compilerOptions.dependencies['entry/Foo']?.path).toBe(path.join(entryModulePath, 'target1.d.ets'));
    expect(config.compilerOptions.dependencies['entry/src/target1/Foo']).toBeDefined();
    expect(config.compilerOptions.dependencies['entry/src/target2/Foo']).toBeUndefined();
    expect(config.compilerOptions.dependencies['entry/src/target3/Foo']).toBeUndefined();
  });

  it('keeps hybrid static files in paths when the package entry is dynamic', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const sourceRoot = path.join(entryModulePath, 'src');
    const entryFile = path.join(sourceRoot, 'Index.ets');
    const staticFile = path.join(sourceRoot, 'StaticPart.ets');
    const declFilesPath = path.join(entryModulePath, 'decl-fileInfo.json');
    await fs.mkdir(sourceRoot, { recursive: true });
    await Promise.all([
      fs.writeFile(entryFile, 'export const entry = true;\n'),
      fs.writeFile(staticFile, "'use static'\nexport const staticPart = true;\n"),
      fs.writeFile(
        declFilesPath,
        JSON.stringify({
          files: {
            'src/Index': {
              declPath: path.join(entryModulePath, 'Index.d.ets'),
              filePath: entryFile,
              ohmUrl: '@normalized:N&&&entry/Index&',
            },
          },
        }),
      ),
    ]);

    const baseConfig = createBuildConfig(projectRootPath);
    const buildConfig: BuildConfig = {
      ...baseConfig,
      externalApiPaths: [],
      interopApiPaths: [],
      dependentModuleList: [
        {
          packageName: 'entry',
          moduleName: 'entry',
          moduleType: 'entry',
          modulePath: entryModulePath,
          sourceRoots: ['src'],
          entryFile: 'src/Index.ets',
          declFilesPath,
          dependencies: [],
          language: 'hybrid',
        },
      ],
    };
    const outputPath = await Pipeline.start<GlueGenContext>()
      .stage(createConfigurationStage())
      .stage(createArkTSConfigStage())
      .run({
        buildConfig,
        logger: SILENT_LOGGER,
        runtime: { nativeExecutablePath: '' },
      });
    const config = JSON.parse(await fs.readFile(outputPath, 'utf8')) as ArkTSConfig;

    expect(config.compilerOptions.paths.entry).toBeUndefined();
    expect(config.compilerOptions.paths['entry/Index']).toBeUndefined();
    expect(config.compilerOptions.paths['entry/src/StaticPart']).toEqual([staticFile]);
    expect(config.compilerOptions.dependencies.entry?.path).toBe(path.join(entryModulePath, 'Index.d.ets'));
    expect(config.compilerOptions.dependencies['entry/Index']).toBeDefined();
    expect(config.compilerOptions.dependencies['entry/src/Index']).toBeDefined();
  });

  it('collects static files only from entryFile and sourceRoots', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const sourceRoot = path.join(entryModulePath, 'src');
    const entryFile = path.join(entryModulePath, 'Entry.ets');
    const sourceFile = path.join(sourceRoot, 'Source.ets');
    const unrelatedFile = path.join(entryModulePath, 'Unrelated.ets');
    await fs.mkdir(sourceRoot, { recursive: true });
    await Promise.all([
      fs.writeFile(entryFile, 'export const entry = true;\n'),
      fs.writeFile(sourceFile, 'export const source = true;\n'),
      fs.writeFile(unrelatedFile, 'export const unrelated = true;\n'),
    ]);

    const baseConfig = createBuildConfig(projectRootPath);
    const buildConfig: BuildConfig = {
      ...baseConfig,
      externalApiPaths: [],
      interopApiPaths: [],
      dependentModuleList: [
        {
          packageName: 'entry',
          moduleName: 'entry',
          moduleType: 'entry',
          modulePath: entryModulePath,
          sourceRoots: ['missing', 'src'],
          entryFile: 'Entry.ets',
          dependencies: [],
          language: '1.2',
        },
      ],
    };
    const outputPath = await Pipeline.start<GlueGenContext>()
      .stage(createConfigurationStage())
      .stage(createArkTSConfigStage())
      .run({
        buildConfig,
        logger: SILENT_LOGGER,
        runtime: { nativeExecutablePath: '' },
      });
    const config = JSON.parse(await fs.readFile(outputPath, 'utf8')) as ArkTSConfig;

    expect(config.compilerOptions.paths['entry/Entry']).toEqual([entryFile]);
    expect(config.compilerOptions.paths['entry/src/Source']).toEqual([sourceFile]);
    expect(config.compilerOptions.paths['entry/Unrelated']).toBeUndefined();
    expect(config.compilerOptions.paths['entry/Index']).toEqual([entryFile]);
    expect(config.compilerOptions.paths.entry).toEqual([sourceRoot, entryModulePath]);
  });
});

function createBuildConfig(projectRootPath: string): BuildConfig {
  return {
    plugins: [],
    buildMode: 'Debug',
    buildType: 'BUILD',
    projectRootPath,
    cachePath: 'cache',
    compileSdkVersion: 10,
    compatibleSdkVersion: 10,
    bundleName: 'com.example.app',
    moduleType: 'entry',
    moduleName: 'entry',
    packageName: 'entry',
    buildSdkPath: path.join(os.tmpdir(), 'gluegen-public-sdk'),
    hasMainModule: true,
    modulePath: path.join(projectRootPath, 'entry'),
    externalApiPaths: [path.join(projectRootPath, 'external-api')],
    byteCodeHar: false,
    interopApiPaths: [path.join(projectRootPath, 'interop-sdk')],
    declgenBridgeConfigPath: path.join(projectRootPath, 'declgen-bridge.json'),
    interopConfigPath: '',
    dependentModuleList: createModuleConfigs(projectRootPath),
  };
}

function createModuleConfigs(projectRootPath: string): readonly ModuleConfig[] {
  return [
    createEntryModuleConfig(projectRootPath),
    createDynamicModuleConfig(projectRootPath),
    createStaticHarConfig(projectRootPath, 'library', ['transitive']),
    createStaticHarConfig(projectRootPath, 'transitive'),
    createStaticHarConfig(projectRootPath, 'unused'),
    createHybridModuleConfig(projectRootPath),
    createUnspecifiedModuleConfig(projectRootPath),
  ];
}

function createEntryModuleConfig(projectRootPath: string): ModuleConfig {
  return {
    packageName: 'entry',
    moduleName: 'entry',
    moduleType: 'entry',
    modulePath: path.join(projectRootPath, 'entry'),
    sourceRoots: ['src/main', 'src/common'],
    entryFile: 'Index.ets',
    interopConfigPath: '',
    dependencies: ['library', 'dynamic-alias', 'hybrid'],
    originalPackageNameMap: {
      'library-alias': 'library',
      'dynamic-alias': 'dynamic',
    },
    language: '1.2',
  };
}

function createDynamicModuleConfig(projectRootPath: string): ModuleConfig {
  const modulePath = path.join(projectRootPath, 'dynamic');
  return {
    packageName: 'dynamic',
    moduleName: 'dynamic',
    moduleType: 'shared',
    bundleType: 'shared',
    bundleName: 'com.example.dynamic',
    modulePath,
    sourceRoots: ['src/main', 'src/common'],
    entryFile: 'src/main/Index.ets',
    declFilesPath: path.join(modulePath, 'decl-fileInfo.json'),
    interopConfigPath: '',
    dependencies: [],
    language: '1.1',
  };
}

function createStaticHarConfig(
  projectRootPath: string,
  packageName: string,
  dependencies: readonly string[] = [],
): ModuleConfig {
  const modulePath = path.join(projectRootPath, packageName);
  return {
    packageName,
    moduleName: packageName,
    moduleType: 'har',
    modulePath,
    sourceRoots: ['src'],
    entryFile: 'Index.ets',
    interopConfigPath: '',
    dependencies,
    abcPath: path.join(modulePath, `${packageName}.abc`),
    language: '1.2',
  };
}

function createHybridModuleConfig(projectRootPath: string): ModuleConfig {
  const modulePath = path.join(projectRootPath, 'hybrid');
  return {
    packageName: 'hybrid',
    moduleName: 'hybrid',
    moduleType: 'har',
    modulePath,
    sourceRoots: ['src'],
    // A directory-valued entryFile means this module has no entry file.
    entryFile: 'entry',
    declFilesPath: path.join(modulePath, 'decl-fileInfo.json'),
    abcPath: path.join(modulePath, 'hybrid.abc'),
    interopConfigPath: '',
    dependencies: [],
    language: 'hybrid',
  };
}

function createUnspecifiedModuleConfig(projectRootPath: string): ModuleConfig {
  return {
    packageName: 'unspecified',
    moduleName: 'unspecified',
    moduleType: 'har',
    modulePath: path.join(projectRootPath, 'unspecified'),
    sourceRoots: ['src'],
    entryFile: 'Index.ets',
    interopConfigPath: '',
    dependencies: [],
  };
}
