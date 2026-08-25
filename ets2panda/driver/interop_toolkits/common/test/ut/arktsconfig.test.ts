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

import {
  ArkTSConfigBuilder,
  ArkTSConfigGenerator,
  createArkTSConfigContext,
  createArkTSConfigSourceContext,
  createDefaultArkTSConfigBuilder,
  type DynamicInteropContext,
  type ModuleInput,
  type ArkTSConfigSourceInput,
} from '../../src/arktsconfig';

interface TestModuleInput extends ModuleInput {
  readonly declgenV2OutPath?: string;
  readonly dynamicFiles: readonly string[];
}

describe('ArkTS config builder', () => {
  let projectRootPath: string;

  beforeEach(() => {
    projectRootPath = fs.mkdtempSync(path.join(os.tmpdir(), 'common-arktsconfig-'));
  });

  afterEach(() => {
    fs.rmSync(projectRootPath, { recursive: true, force: true });
  });

  it('builds module and SDK contributions from flattened context data', async () => {
    const entryPath = path.join(projectRootPath, 'entry');
    const dynamicPath = path.join(projectRootPath, 'dynamic');
    const hybridPath = path.join(projectRootPath, 'hybrid');
    const externalApiPath = path.join(projectRootPath, 'external-api');
    const interopApiPath = path.join(projectRootPath, 'interop-api');
    const entryFile = path.join(entryPath, 'Index.ets');
    const staticFile = path.join(entryPath, 'src', 'Main.ets');
    const dynamicFile = path.join(dynamicPath, 'src', 'main', 'Index.ets');
    const declgenV2OutPath = path.join(projectRootPath, 'declgen', 'dynamic');
    const libraryPath = path.join(projectRootPath, 'library');
    const transitivePath = path.join(projectRootPath, 'transitive');
    const unusedPath = path.join(projectRootPath, 'unused');

    for (const directory of [
      path.dirname(staticFile),
      path.dirname(dynamicFile),
      hybridPath,
      path.join(externalApiPath, 'nested'),
      path.join(externalApiPath, 'arkui', 'runtime-api', 'components'),
      path.join(interopApiPath, 'api', 'subdir'),
      path.join(interopApiPath, 'api', 'arkui', 'runtime-api', 'nested'),
      path.join(interopApiPath, 'api'),
      path.join(interopApiPath, 'component'),
      path.join(interopApiPath, 'kits'),
      path.join(libraryPath, 'src'),
      path.join(transitivePath, 'src'),
      unusedPath,
    ]) {
      fs.mkdirSync(directory, { recursive: true });
    }
    fs.writeFileSync(entryFile, "'use static'\n");
    fs.writeFileSync(staticFile, 'export const value = true;\n');
    fs.writeFileSync(dynamicFile, 'export const dynamicValue = true;\n');
    fs.writeFileSync(path.join(libraryPath, 'Index.ets'), "'use static'\n");
    fs.writeFileSync(path.join(libraryPath, 'src', 'Library.ets'), 'export const library = true;\n');
    fs.writeFileSync(path.join(transitivePath, 'Index.ets'), "'use static'\n");
    fs.writeFileSync(path.join(transitivePath, 'src', 'Types.d.ets'), 'export declare const value: boolean;\n');
    fs.writeFileSync(path.join(unusedPath, 'Index.ets'), "'use static'\n");
    fs.writeFileSync(path.join(externalApiPath, '@ohos.sample.d.ets'), 'export declare const sample: string;\n');
    fs.writeFileSync(path.join(externalApiPath, 'nested', 'feature.d.ets'), 'export declare const feature: string;\n');
    fs.writeFileSync(
      path.join(externalApiPath, 'arkui', 'runtime-api', 'components', 'Widget.d.ets'),
      'export declare class Widget {}\n',
    );
    fs.writeFileSync(path.join(externalApiPath, 'ignored.ets'), 'export const ignored = true;\n');
    fs.writeFileSync(
      path.join(interopApiPath, 'api', '@ohos.storage.d.ets'),
      'export declare const storage: string;\n',
    );
    fs.writeFileSync(
      path.join(interopApiPath, 'api', '@system.router.d.ets'),
      'export declare const router: string;\n',
    );
    fs.writeFileSync(
      path.join(interopApiPath, 'api', 'subdir', 'normal.d.ets'),
      'export declare const normal: string;\n',
    );
    fs.writeFileSync(
      path.join(interopApiPath, 'api', 'arkui', 'runtime-api', 'nested', 'Runtime.d.ets'),
      'export declare const runtime: string;\n',
    );
    fs.writeFileSync(path.join(interopApiPath, 'component', 'button.d.ets'), 'export declare const button: string;\n');
    fs.writeFileSync(
      path.join(interopApiPath, 'kits', '@kit.Ignored.d.ets'),
      'export declare const ignored: string;\n',
    );
    fs.writeFileSync(path.join(interopApiPath, 'api', 'ignored.ets'), 'export const ignored = true;\n');

    const modules: readonly TestModuleInput[] = [
      {
        packageName: 'entry',
        moduleName: 'entry',
        moduleType: 'entry',
        modulePath: entryPath,
        sourceRoots: ['src'],
        entryFile: 'Index.ets',
        language: '1.2',
        dependencies: ['library-alias', 'dynamic-alias', 'hybrid'],
        originalPackageNameMap: { 'library-alias': 'library', 'dynamic-alias': 'dynamic' },
        dynamicFiles: [],
      },
      {
        packageName: 'dynamic',
        moduleName: 'dynamic',
        moduleType: 'shared',
        bundleType: 'shared',
        bundleName: 'com.example.dynamic',
        modulePath: dynamicPath,
        sourceRoots: ['src/main'],
        entryFile: 'src/main/Index.ets',
        language: '1.1',
        dependencies: [],
        declgenV2OutPath,
        dynamicFiles: [dynamicFile],
      },
      {
        packageName: 'library',
        moduleName: 'library',
        moduleType: 'har',
        modulePath: libraryPath,
        sourceRoots: ['src'],
        entryFile: 'Index.ets',
        language: '1.2',
        dependencies: ['transitive'],
        abcPath: path.join(libraryPath, 'library.abc'),
        dynamicFiles: [],
      },
      {
        packageName: 'transitive',
        moduleName: 'transitive',
        moduleType: 'har',
        modulePath: transitivePath,
        sourceRoots: ['src'],
        entryFile: 'Index.ets',
        language: '1.2',
        dependencies: [],
        abcPath: path.join(transitivePath, 'transitive.abc'),
        dynamicFiles: [],
      },
      {
        packageName: 'unused',
        moduleName: 'unused',
        moduleType: 'har',
        modulePath: unusedPath,
        sourceRoots: [],
        entryFile: 'Index.ets',
        language: '1.2',
        dependencies: [],
        abcPath: path.join(unusedPath, 'unused.abc'),
        dynamicFiles: [],
      },
      {
        packageName: 'hybrid',
        moduleName: 'hybrid',
        moduleType: 'har',
        modulePath: hybridPath,
        sourceRoots: [],
        entryFile: '',
        language: 'hybrid',
        dependencies: [],
        dynamicFiles: [],
      },
    ];
    const source = createArkTSConfigSourceContext(
      createSourceInput(projectRootPath, modules, externalApiPath, interopApiPath),
    );

    expect(source.interopContexts.has('entry')).toBe(false);
    expect(source.interopContexts.get('hybrid')).toEqual({
      packageName: 'hybrid',
      files: {},
    });
    expect(source.interopContexts.get('dynamic')).toEqual({
      packageName: 'dynamic',
      files: {
        'src/main/Index': {
          declPath: path.join(declgenV2OutPath, 'src/main/Index.d.ets'),
          filePath: dynamicFile,
          ohmUrl: '@normalized:N&&&dynamic/src/main/Index&',
        },
      },
    });

    const config = await createDefaultArkTSConfigBuilder(createArkTSConfigContext(source)).build();
    expect(config.compilerOptions.package).toBe('entry');
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
    expect(config.compilerOptions.baseUrl).toBe(entryPath);
    expect(config.compilerOptions.rootDir).toBe(projectRootPath);
    expect(config.compilerOptions.cacheDir).toBe(path.join(projectRootPath, 'cache'));
    expect(config.compilerOptions.declgenV2OutPath).toBe(config.compilerOptions.cacheDir);
    expect(config.compilerOptions.paths.entry).toEqual([path.join(entryPath, 'src'), entryPath]);
    expect(config.compilerOptions.paths['entry/src/Main']).toEqual([staticFile]);
    expect(config.compilerOptions.paths['@ohos.sample']).toEqual([path.join(externalApiPath, '@ohos.sample')]);
    expect(config.compilerOptions.paths['nested.feature']).toEqual([path.join(externalApiPath, 'nested', 'feature')]);
    expect(config.compilerOptions.paths.Widget).toEqual([
      path.join(externalApiPath, 'arkui', 'runtime-api', 'components', 'Widget'),
    ]);
    expect(config.compilerOptions.paths.ignored).toBeUndefined();
    expect(config.compilerOptions.paths.library).toEqual([path.join(libraryPath, 'src'), libraryPath]);
    expect(config.compilerOptions.paths.transitive).toEqual([path.join(transitivePath, 'src'), transitivePath]);
    expect(config.compilerOptions.paths.unused).toBeUndefined();
    expect(config.compilerOptions.dependencies.library).toEqual({
      language: 'ets',
      path: path.join(libraryPath, 'library.abc'),
      ohmUrl: 'library',
      mainFile: 'Index',
    });
    expect(config.compilerOptions.dependencies['library-alias']).toEqual(config.compilerOptions.dependencies.library);
    expect(config.compilerOptions.dependencies.transitive).toEqual({
      language: 'ets',
      path: path.join(transitivePath, 'transitive.abc'),
      ohmUrl: 'transitive',
      mainFile: 'Index',
    });
    expect(config.compilerOptions.dependencies.unused).toBeUndefined();

    const dynamicEntry = {
      language: 'js',
      path: path.join(declgenV2OutPath, 'src/main/Index.d.ets'),
      sourceFilePath: dynamicFile,
      ohmUrl: '@normalized:N&dynamic&com.example.dynamic&dynamic/src/main/Index&',
    };
    expect(config.compilerOptions.dependencies.dynamic).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/src/main/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic-alias']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic-alias/Index']).toEqual(dynamicEntry);
    expect(config.compilerOptions.dependencies['dynamic/@ohos.storage']).toEqual({
      language: 'js',
      path: path.join(interopApiPath, 'api', '@ohos.storage.d.ets'),
      ohmUrl: '@ohos:storage',
      alias: ['@ohos.storage', 'dynamic@ohos.storage'],
    });
    expect(config.compilerOptions.dependencies['dynamic/@system.router']).toEqual({
      language: 'js',
      path: path.join(interopApiPath, 'api', '@system.router.d.ets'),
      ohmUrl: '@native:system.router',
      alias: ['@system.router', 'dynamic@system.router'],
    });
    expect(config.compilerOptions.dependencies['dynamic/subdir/normal']).toEqual({
      language: 'js',
      path: path.join(interopApiPath, 'api', 'subdir', 'normal.d.ets'),
      ohmUrl: '',
      alias: ['subdir/normal', 'dynamicsubdir/normal'],
    });
    expect(config.compilerOptions.dependencies['dynamic/Runtime']?.path).toBe(
      path.join(interopApiPath, 'api', 'arkui', 'runtime-api', 'nested', 'Runtime.d.ets'),
    );
    expect(config.compilerOptions.dependencies['component/button']).toEqual({
      language: 'js',
      path: path.join(interopApiPath, 'component', 'button.d.ets'),
      ohmUrl: '',
      alias: ['button', 'dynamicbutton'],
    });
    expect(config.compilerOptions.dependencies['dynamic/@kit.Ignored']).toBeUndefined();
    expect(config.compilerOptions.dependencies['dynamic/ignored']).toBeUndefined();
    expect(config.compilerOptions.dependencies['std/core']).toEqual({
      language: 'ets',
      path: path.join(projectRootPath, 'panda', 'lib', 'etsstdlib.abc'),
      ohmUrl: 'std/core',
    });
  });

  it('runs asynchronous rules in apply order', async () => {
    const calls: string[] = [];
    let releaseFirstRule: () => void = () => undefined;
    const firstRulePending = new Promise<void>((resolve) => {
      releaseFirstRule = resolve;
    });
    const builder = new ArkTSConfigBuilder({
      packageName: 'entry',
      baseUrl: projectRootPath,
      rootDir: projectRootPath,
      cachePath: path.join(projectRootPath, 'cache'),
    })
      .apply({
        generate: async () => {
          calls.push('first');
          await firstRulePending;
          return { paths: new Map([['entry', ['first']]]), dependencies: new Map() };
        },
      })
      .apply({
        generate: async () => {
          calls.push('second');
          return { paths: new Map([['entry', ['second']]]), dependencies: new Map() };
        },
      });

    const configPending = builder.build();
    await Promise.resolve();
    expect(calls).toEqual(['first']);
    releaseFirstRule();

    const config = await configPending;
    expect(calls).toEqual(['first', 'second']);
    expect(config.compilerOptions.paths.entry).toEqual(['first', 'second']);
  });

  it('writes the generated config asynchronously', async () => {
    const modulePath = path.join(projectRootPath, 'entry');
    const entryFile = path.join(modulePath, 'Index.ets');
    fs.mkdirSync(modulePath, { recursive: true });
    fs.writeFileSync(entryFile, "'use static'\n");
    const source = createArkTSConfigSourceContext(
      createSingleModuleSourceInput(projectRootPath, {
        packageName: 'entry',
        modulePath,
        sourceRoots: [],
        entryFile: 'Index.ets',
        language: '1.2',
        dynamicFiles: [],
      }),
    );
    const outputPath = path.join(projectRootPath, 'output', 'arktsconfig.json');

    await new ArkTSConfigGenerator(source).write(outputPath);

    const config = JSON.parse(fs.readFileSync(outputPath, 'utf8')) as { compilerOptions: { package: string } };
    expect(config.compilerOptions.package).toBe('entry');
  });

  it('adds the entry module declarations when the entry module uses ArkTS 1.1', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const entryFile = path.join(entryModulePath, 'Index.ets');
    const declgenV2OutPath = path.join(projectRootPath, 'declgen', 'entry');
    fs.mkdirSync(entryModulePath, { recursive: true });
    fs.writeFileSync(entryFile, 'export const dynamicEntry = true;\n');

    const module: TestModuleInput = {
      packageName: 'entry',
      moduleName: 'entry',
      moduleType: 'entry',
      modulePath: entryModulePath,
      sourceRoots: [],
      entryFile: 'Index.ets',
      language: '1.1',
      dependencies: [],
      declgenV2OutPath,
      dynamicFiles: [entryFile],
    };

    const config = await buildArkTSConfig(createSingleModuleSourceInput(projectRootPath, module));
    const expectedEntry = {
      language: 'js',
      path: path.join(declgenV2OutPath, 'Index.d.ets'),
      sourceFilePath: entryFile,
      ohmUrl: '@normalized:N&&&entry/Index&',
    };
    expect(config.compilerOptions.dependencies.entry).toEqual(expectedEntry);
    expect(config.compilerOptions.dependencies['entry/Index']).toEqual(expectedEntry);
  });

  it('lets the highest-priority sourceRoot claim a dynamic transformed key', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const declgenV2OutPath = path.join(projectRootPath, 'declgen', 'entry');
    const entryFile = path.join(entryModulePath, 'src', 'target1', 'Index.ets');
    const dynamicFiles = ['target3', 'target2', 'target1'].map((target) =>
      path.join(entryModulePath, 'src', target, 'Foo.ets'),
    );
    for (const file of [...dynamicFiles, entryFile]) {
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, 'export const value = true;\n');
    }

    const module: TestModuleInput = {
      packageName: 'entry',
      moduleName: 'entry',
      moduleType: 'entry',
      modulePath: entryModulePath,
      sourceRoots: ['src/target3', 'src/target2', 'src/target1'],
      entryFile: 'src/target1/Index.ets',
      language: '1.1',
      dependencies: [],
      declgenV2OutPath,
      dynamicFiles: [...dynamicFiles, entryFile],
    };

    const config = await buildArkTSConfig(createSingleModuleSourceInput(projectRootPath, module));
    expect(config.compilerOptions.dependencies['entry/Foo']?.path).toBe(
      path.join(declgenV2OutPath, 'src', 'target1', 'Foo.d.ets'),
    );
    expect(config.compilerOptions.dependencies['entry/src/target1/Foo']).toBeDefined();
    expect(config.compilerOptions.dependencies['entry/src/target2/Foo']).toBeUndefined();
    expect(config.compilerOptions.dependencies['entry/src/target3/Foo']).toBeUndefined();
  });

  it('keeps the hybrid bare-package path without adding a static Index path for a dynamic entry', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const sourceRoot = path.join(entryModulePath, 'src');
    const additionalSourceRoot = path.join(entryModulePath, 'generated');
    const entryFile = path.join(sourceRoot, 'Index.ets');
    const staticFile = path.join(sourceRoot, 'StaticPart.ets');
    const declgenV2OutPath = path.join(projectRootPath, 'declgen', 'entry');
    fs.mkdirSync(sourceRoot, { recursive: true });
    fs.mkdirSync(additionalSourceRoot, { recursive: true });
    fs.writeFileSync(entryFile, 'export const entry = true;\n');
    fs.writeFileSync(staticFile, "'use static'\nexport const staticPart = true;\n");

    const module: TestModuleInput = {
      packageName: 'entry',
      moduleName: 'entry',
      moduleType: 'entry',
      modulePath: entryModulePath,
      sourceRoots: ['src', 'generated'],
      entryFile: 'src/Index.ets',
      language: 'hybrid',
      dependencies: [],
      declgenV2OutPath,
      dynamicFiles: [entryFile],
    };

    const config = await buildArkTSConfig(createSingleModuleSourceInput(projectRootPath, module));
    expect(config.compilerOptions.paths.entry).toEqual([additionalSourceRoot, sourceRoot, entryModulePath]);
    expect(config.compilerOptions.paths['entry/Index']).toBeUndefined();
    expect(config.compilerOptions.paths['entry/src/StaticPart']).toEqual([staticFile]);
    expect(config.compilerOptions.dependencies.entry?.path).toBe(path.join(declgenV2OutPath, 'src', 'Index.d.ets'));
    expect(config.compilerOptions.dependencies['entry/Index']).toBeDefined();
    expect(config.compilerOptions.dependencies['entry/src/Index']).toBeDefined();
  });

  it('collects static files only from entryFile and sourceRoots', async () => {
    const entryModulePath = path.join(projectRootPath, 'entry');
    const sourceRoot = path.join(entryModulePath, 'src');
    const entryFile = path.join(entryModulePath, 'Entry.ets');
    const sourceFile = path.join(sourceRoot, 'Source.ets');
    const unrelatedFile = path.join(entryModulePath, 'Unrelated.ets');
    fs.mkdirSync(sourceRoot, { recursive: true });
    fs.writeFileSync(entryFile, 'export const entry = true;\n');
    fs.writeFileSync(sourceFile, 'export const source = true;\n');
    fs.writeFileSync(unrelatedFile, 'export const unrelated = true;\n');

    const module: TestModuleInput = {
      packageName: 'entry',
      moduleName: 'entry',
      moduleType: 'entry',
      modulePath: entryModulePath,
      sourceRoots: ['missing', 'src'],
      entryFile: 'Entry.ets',
      language: '1.2',
      dependencies: [],
      dynamicFiles: [],
    };

    const config = await buildArkTSConfig(createSingleModuleSourceInput(projectRootPath, module));
    expect(config.compilerOptions.paths['entry/Entry']).toEqual([entryFile]);
    expect(config.compilerOptions.paths['entry/src/Source']).toEqual([sourceFile]);
    expect(config.compilerOptions.paths['entry/Unrelated']).toBeUndefined();
    expect(config.compilerOptions.paths['entry/Index']).toEqual([entryFile]);
    expect(config.compilerOptions.paths.entry).toEqual([sourceRoot, entryModulePath]);
  });
});

function buildArkTSConfig(input: ArkTSConfigSourceInput) {
  const source = createArkTSConfigSourceContext(input);
  return createDefaultArkTSConfigBuilder(createArkTSConfigContext(source)).build();
}

function createSingleModuleSourceInput(projectRootPath: string, module: TestModuleInput): ArkTSConfigSourceInput {
  return createSourceInput(
    projectRootPath,
    [module],
    path.join(projectRootPath, 'missing-external-api'),
    path.join(projectRootPath, 'missing-interop-api'),
  );
}

function createSourceInput(
  projectRootPath: string,
  dependentModuleList: readonly TestModuleInput[],
  externalApiPath: string,
  interopApiPath: string,
): ArkTSConfigSourceInput {
  return {
    projectRootPath,
    cachePath: path.join(projectRootPath, 'cache'),
    pandaSdkPath: path.join(projectRootPath, 'panda'),
    packageName: 'entry',
    bundleName: 'com.example.application',
    dependentModuleList,
    interopContexts: createTestInteropContexts(dependentModuleList),
    externalApiPaths: [externalApiPath],
    dynamicInteropSdkPaths: [interopApiPath],
  };
}

function createTestInteropContexts(modules: readonly TestModuleInput[]): ReadonlyMap<string, DynamicInteropContext> {
  const contexts = new Map<string, DynamicInteropContext>();
  for (const module of modules) {
    if (module.language !== '1.1' && module.language !== 'hybrid') {
      continue;
    }
    const files: Record<string, DynamicInteropContext['files'][string]> = {};
    for (const filePath of module.dynamicFiles) {
      const normalizedFilePath = filePath.replace(/\\/g, '/');
      const normalizedModulePath = module.modulePath.replace(/\\/g, '/');
      const projectFilePath = normalizedFilePath
        .replace(/\.(?:d\.)?[^/.]+$/, '')
        .replace(`${normalizedModulePath}/`, '');
      files[projectFilePath] = {
        declPath: `${path.join(module.declgenV2OutPath ?? '', projectFilePath).replace(/\\/g, '/')}.d.ets`,
        filePath,
        ohmUrl: `@normalized:N&&&${module.packageName}/${projectFilePath}&`,
      };
    }
    contexts.set(module.packageName, { packageName: module.packageName, files });
  }
  return contexts;
}
