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
import { AggregateUserError, InteropConfigError } from '../../../src/interop-config/errors';
import type { InteropConfigHost } from '../../../src/interop-config/host';
import { resolveInteropConfig as resolve } from '../../../src/interop-config/resolve';
import type {
  InteropConfigModuleInfo,
  InteropFileEntry,
  MainModuleInfo,
  ModuleTable,
} from '../../../src/interop-config/types';

const FIXTURE_CONFIG_PATH = path.join(__dirname, 'interop-config.json5');
const MERGE_MAIN_CONFIG_PATH = path.join(__dirname, 'merge-main.json5');
const MERGE_CONTRIBUTOR_CONFIG_PATH = path.join(__dirname, 'merge-contributor.json5');
const PACKAGE_PRIORITY_CONFIG_PATH = path.join(__dirname, 'package-priority.json5');
const UNREACHABLE_PACKAGE_CONFIG_PATH = path.join(__dirname, 'unreachable-package.json5');

describe('resolve interop configuration', () => {
  let projectRootPath: string;

  beforeEach(() => {
    projectRootPath = fs.mkdtempSync(path.join(os.tmpdir(), 'build-system-interop-config-'));
  });

  afterEach(() => {
    fs.rmSync(projectRootPath, { recursive: true, force: true });
  });

  it('resolves static and dynamic entries for modules and selected dependencies', () => {
    const mainModule = createMainModule('main-package', ['package-full', 'package-partial'], FIXTURE_CONFIG_PATH);
    const fullModule = createModule('package-full');
    const partialModule = createModule('package-partial');
    const mainStaticFile = createFile(mainModule, 'src/main.ets');
    const mainDynamicFile = createFile(mainModule, 'src/main.ts');
    const partialStaticFile = createFile(partialModule, 'src/partial.ets');
    const partialDynamicFile = createFile(partialModule, 'src/partial.ts');

    const targets = resolve(createModuleTable(mainModule, fullModule, partialModule));

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [entry('main-package', 'src/main.ets', mainStaticFile)],
      dynamicFiles: [entry('main-package', 'src/main.ts', mainDynamicFile)],
    });
    expect(targets.get(partialModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [entry('package-partial', 'src/partial.ets', partialStaticFile)],
      dynamicFiles: [entry('package-partial', 'src/partial.ts', partialDynamicFile)],
    });
    expect(targets.get(fullModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: fullModule,
    });
  });

  it('merges static and dynamic item contributions for the same package', () => {
    const mainModule = createMainModule('main-package', ['package-contributor'], MERGE_MAIN_CONFIG_PATH);
    const contributorModule = createModule('package-contributor', ['package-target'], MERGE_CONTRIBUTOR_CONFIG_PATH);
    const targetModule = createModule('package-target');
    const sharedStaticFile = createFile(targetModule, 'src/shared.ets');
    const mainStaticFile = createFile(targetModule, 'src/from-main.ets');
    const contributorStaticFile = createFile(targetModule, 'src/from-contributor.ets');
    const sharedDynamicFile = createFile(targetModule, 'src/shared.ts');
    const mainDynamicFile = createFile(targetModule, 'src/from-main.ts');
    const contributorDynamicFile = createFile(targetModule, 'src/from-contributor.ts');

    const targets = resolve(createModuleTable(mainModule, contributorModule, targetModule));

    expect(targets.get(targetModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [
        entry('package-target', 'src/shared.ets', sharedStaticFile),
        entry('package-target', 'src/from-main.ets', mainStaticFile),
        entry('package-target', 'src/from-contributor.ets', contributorStaticFile),
      ],
      dynamicFiles: [
        entry('package-target', 'src/shared.ts', sharedDynamicFile),
        entry('package-target', 'src/from-main.ts', mainDynamicFile),
        entry('package-target', 'src/from-contributor.ts', contributorDynamicFile),
      ],
    });
  });

  it('prefers a package target over item contributions for the same package', () => {
    const mainModule = createMainModule('main-package', ['package-priority'], MERGE_MAIN_CONFIG_PATH);
    const priorityModule = createModule('package-priority', ['package-target'], PACKAGE_PRIORITY_CONFIG_PATH);
    const targetModule = createModule('package-target');
    createFile(targetModule, 'src/shared.ets');
    createFile(targetModule, 'src/from-main.ets');
    createFile(targetModule, 'src/shared.ts');
    createFile(targetModule, 'src/from-main.ts');

    const targets = resolve(createModuleTable(mainModule, priorityModule, targetModule));

    expect(targets.get(targetModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: targetModule,
    });
  });

  it('resolves a dependency package that is present in the module table but not reachable from main', () => {
    const mainModule = createMainModule('main-package', [], UNREACHABLE_PACKAGE_CONFIG_PATH);
    const unreachableModule = createModule('package-unreachable');

    const targets = resolve(createModuleTable(mainModule, unreachableModule));

    expect(targets.get(unreachableModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: unreachableModule,
    });
  });

  it('reports the interop config path when a static file has an unsupported extension', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/main.js'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'unsupported extension',
      position: configPath,
    });
  });

  it('reports the interop config path when a static file cannot be accessed', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/missing.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'cannot be accessed',
      position: configPath,
    });
  });

  it('reports the interop config path when a static path is not a regular file', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/directory.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    fs.mkdirSync(path.join(mainModule.modulePath, 'src', 'directory.ets'), { recursive: true });

    expectConfigError(createModuleTable(mainModule), {
      description: 'is not a regular file',
      position: configPath,
    });
  });

  it('reports the interop config path when a static file is outside its module root', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['../outside.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    fs.writeFileSync(path.join(projectRootPath, 'outside.ets'), '', 'utf8');

    expectConfigError(createModuleTable(mainModule), {
      description: 'outside its module root',
      position: configPath,
    });
  });

  it('reports the interop config path when a dynamic file has an unsupported extension', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { dynamic: ['src/main.txt'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'unsupported extension',
      position: configPath,
    });
  });

  it('accepts a dynamic interop file with a .js extension', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { dynamic: ['src/main.js'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    const dynamicFile = createFile(mainModule, 'src/main.js');

    const targets = resolve(createModuleTable(mainModule));

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [],
      dynamicFiles: [entry('main-package', 'src/main.js', dynamicFile)],
    });
  });

  it('reports the interop config path when a dynamic file cannot be accessed', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { dynamic: ['src/missing.ts'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'cannot be accessed',
      position: configPath,
    });
  });

  it('reports the declaring config path for files selected via a dependency source', () => {
    const mainConfigPath = writeConfig(
      'main-package',
      `{ interopEntries: { dependency: { source: { 'package-target': { static: ['src/missing.ets'] } } } } }`,
    );
    const targetConfigPath = writeConfig('package-target', `{ interopEntries: { static: ['src/ok.ets'] } }`);
    const mainModule = createMainModule('main-package', ['package-target'], mainConfigPath);
    const targetModule = createModule('package-target', [], targetConfigPath);
    createFile(targetModule, 'src/ok.ets');

    expectConfigError(createModuleTable(mainModule, targetModule), {
      description: 'cannot be accessed',
      position: mainConfigPath,
    });
  });

  it('rejects a dynamic source file declared as a static interop entry', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/main.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    const staticFile = createFile(mainModule, 'src/main.ets');
    const getLanguageFromSourceCode = jest.fn((_filePath: string): LANGUAGE_VERSION => LANGUAGE_VERSION.ARKTS_1_1);

    expectConfigError(
      createModuleTable(mainModule),
      {
        description: 'Invalid interop configuration',
        cause: 'src/main.ets is a dynamic source file, but is configured as a static interop entry.',
        position: configPath,
      },
      { getLanguageFromSourceCode },
    );
    expect(getLanguageFromSourceCode).toHaveBeenCalledWith(staticFile);
  });

  it('rejects a static source file declared as a dynamic interop entry', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { dynamic: ['src/main.ts'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    createFile(mainModule, 'src/main.ts');

    expectConfigError(
      createModuleTable(mainModule),
      {
        description: 'Invalid interop configuration',
        cause: 'src/main.ts is a static source file, but is configured as a dynamic interop entry.',
        position: configPath,
      },
      { getLanguageFromSourceCode: (_filePath: string): LANGUAGE_VERSION => LANGUAGE_VERSION.ARKTS_1_2 },
    );
  });

  it('resolves entries whose language matches the host classification', () => {
    const configPath = writeConfig(
      'main-package',
      `{ interopEntries: { static: ['src/main.ets'], dynamic: ['src/main.ts'] } }`,
    );
    const mainModule = createMainModule('main-package', [], configPath);
    const staticFile = createFile(mainModule, 'src/main.ets');
    const dynamicFile = createFile(mainModule, 'src/main.ts');
    const languages = new Map<string, LANGUAGE_VERSION>([
      [staticFile, LANGUAGE_VERSION.ARKTS_1_2],
      [dynamicFile, LANGUAGE_VERSION.ARKTS_1_1],
    ]);

    const targets = resolve(createModuleTable(mainModule), {
      getLanguageFromSourceCode: (filePath: string): LANGUAGE_VERSION =>
        languages.get(filePath) ?? LANGUAGE_VERSION.ARKTS_1_2,
    });

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [entry('main-package', 'src/main.ets', staticFile)],
      dynamicFiles: [entry('main-package', 'src/main.ts', dynamicFile)],
    });
  });

  it('treats a hybrid classification as non-conflicting', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/main.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    const staticFile = createFile(mainModule, 'src/main.ets');

    const targets = resolve(createModuleTable(mainModule), {
      getLanguageFromSourceCode: (_filePath: string): LANGUAGE_VERSION => LANGUAGE_VERSION.ARKTS_HYBRID,
    });

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [entry('main-package', 'src/main.ets', staticFile)],
      dynamicFiles: [],
    });
  });

  it('rejects a misclassified dependency-source entry at the declaring config', () => {
    const mainConfigPath = writeConfig(
      'main-package',
      `{ interopEntries: { dependency: { source: { 'package-target': { static: ['src/main.ets'] } } } } }`,
    );
    const targetModule = createModule('package-target');
    createFile(targetModule, 'src/main.ets');
    const mainModule = createMainModule('main-package', ['package-target'], mainConfigPath);

    expectConfigError(
      createModuleTable(mainModule, targetModule),
      {
        description: 'Invalid interop configuration',
        cause:
          'src/main.ets in dependency package-target is a dynamic source file, ' +
          'but is configured as a static interop entry.',
        position: mainConfigPath,
      },
      { getLanguageFromSourceCode: (_filePath: string): LANGUAGE_VERSION => LANGUAGE_VERSION.ARKTS_1_1 },
    );
  });

  it('accepts a static entry whose source code carries the static directive via the default host', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/main.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    const staticFile = createFile(mainModule, 'src/main.ets');
    fs.writeFileSync(staticFile, "'use static'\n", 'utf8');

    const targets = resolve(createModuleTable(mainModule), {});

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [entry('main-package', 'src/main.ets', staticFile)],
      dynamicFiles: [],
    });
  });

  it('rejects a static entry whose source code lacks the static directive via the default host', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/main.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    createFile(mainModule, 'src/main.ets');

    expectConfigError(
      createModuleTable(mainModule),
      {
        description: 'Invalid interop configuration',
        cause: 'src/main.ets is a dynamic source file, but is configured as a static interop entry.',
        position: configPath,
      },
      {},
    );
  });

  it('aggregates language conflicts across multiple misconfigured files', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: ['src/a.ets', 'src/b.ets'] } }`);
    const mainModule = createMainModule('main-package', [], configPath);
    createFile(mainModule, 'src/a.ets');
    createFile(mainModule, 'src/b.ets');

    const aggregate = captureError(createModuleTable(mainModule), {});

    expect(aggregate).toBeInstanceOf(AggregateUserError);
    const errors = (aggregate as AggregateUserError).errors;
    expect(errors).toHaveLength(2);
    expect(errors.map((error) => (error as InteropConfigError).errorMessage.cause)).toEqual([
      'src/a.ets is a dynamic source file, but is configured as a static interop entry.',
      'src/b.ets is a dynamic source file, but is configured as a static interop entry.',
    ]);
    for (const error of errors) {
      expect(error).toBeInstanceOf(InteropConfigError);
      expect((error as InteropConfigError).errorMessage.position).toBe(configPath);
    }
  });

  it('aggregates mixed file validation errors and reports every misconfigured file', () => {
    const configPath = writeConfig(
      'main-package',
      `{ interopEntries: { static: ['src/missing.ets', 'src/conflict.ets'] } }`,
    );
    const mainModule = createMainModule('main-package', [], configPath);
    createFile(mainModule, 'src/conflict.ets');

    const aggregate = captureError(createModuleTable(mainModule), {});

    expect(aggregate).toBeInstanceOf(AggregateUserError);
    const errors = (aggregate as AggregateUserError).errors;
    expect(errors).toHaveLength(2);
    expect(errors[0]).toBeInstanceOf(InteropConfigError);
    expect((errors[0] as InteropConfigError).errorMessage.description).toContain('cannot be accessed');
    expect((errors[1] as InteropConfigError).errorMessage.cause).toBe(
      'src/conflict.ets is a dynamic source file, but is configured as a static interop entry.',
    );
  });

  it('aggregates conflicts from own entries and dependency sources across modules', () => {
    const mainConfigPath = writeConfig(
      'main-package',
      `{ interopEntries: { static: ['src/own.ets'], dependency: { source: { 'package-target': { static: ['src/dep.ets'] } } } } }`,
    );
    const targetModule = createModule('package-target');
    createFile(targetModule, 'src/dep.ets');
    const mainModule = createMainModule('main-package', ['package-target'], mainConfigPath);
    createFile(mainModule, 'src/own.ets');

    const aggregate = captureError(createModuleTable(mainModule, targetModule), {});

    expect(aggregate).toBeInstanceOf(AggregateUserError);
    const errors = (aggregate as AggregateUserError).errors;
    expect(errors).toHaveLength(2);
    const causes = errors.map((error) => (error as InteropConfigError).errorMessage.cause);
    expect(causes).toContain('src/own.ets is a dynamic source file, but is configured as a static interop entry.');
    expect(causes).toContain(
      'src/dep.ets in dependency package-target is a dynamic source file, but is configured as a static interop entry.',
    );
  });

  it('reports a config file that cannot be read', () => {
    const configPath = path.join(projectRootPath, 'main-package', 'interop-config.json5');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'could not be read',
      position: configPath,
    });
  });

  it('reports a config that is not valid JSON5', () => {
    const configPath = writeConfig('main-package', '{ invalid');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'could not be parsed',
      position: configPath,
    });
  });

  it('reports a config whose root is not an object', () => {
    const configPath = writeConfig('main-package', '42');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The interop configuration must be a JSON5 object.',
      position: configPath,
    });
  });

  it('reports an interopEntries field that is not an object', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: 42 }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries" field must be an object.',
      position: configPath,
    });
  });

  it('reports a static entries field that is not an array', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { static: 'src/a.ets' } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.static" field must be an array of strings.',
      position: configPath,
    });
  });

  it('reports a static entry that is not a string', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: { static: [42] } }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.static" field must be an array of strings.',
      position: configPath,
    });
  });

  it('reports a dynamic entries field that is not an array', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: { dynamic: {} } }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dynamic" field must be an array of strings.',
      position: configPath,
    });
  });

  it('reports a dependency field that is not an object', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: { dependency: [] } }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dependency" field must be an object.',
      position: configPath,
    });
  });

  it('reports a dependency package list that is not an array of strings', () => {
    const configPath = writeConfig('main-package', `{ interopEntries: { dependency: { package: 'libFoo' } } }`);
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dependency.package" field must be an array of strings.',
      position: configPath,
    });
  });

  it('reports a dependency source field that is not an object', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: { dependency: { source: [] } } }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dependency.source" field must be an object.',
      position: configPath,
    });
  });

  it('reports a dependency source selection that is not an object', () => {
    const configPath = writeConfig('main-package', '{ interopEntries: { dependency: { source: { libFoo: [] } } } }');
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dependency.source.libFoo" field must be an object.',
      position: configPath,
    });
  });

  it('reports a dependency source static list that is not an array of strings', () => {
    const configPath = writeConfig(
      'main-package',
      '{ interopEntries: { dependency: { source: { libFoo: { static: 42 } } } } }',
    );
    const mainModule = createMainModule('main-package', [], configPath);

    expectConfigError(createModuleTable(mainModule), {
      description: 'Invalid interop configuration',
      cause: 'The "interopEntries.dependency.source.libFoo.static" field must be an array of strings.',
      position: configPath,
    });
  });

  function entry(packageName: string, relativePath: string, filePath: string): InteropFileEntry {
    return { packageName, relativePath, filePath };
  }

  function createMainModule(
    packageName: string,
    dependencies: readonly string[],
    interopConfigPath: string,
  ): MainModuleInfo {
    const modulePath = path.join(projectRootPath, packageName);
    return {
      ...createModule(packageName, dependencies, interopConfigPath),
      projectRootPath,
      cachePath: path.join(projectRootPath, 'cache'),
      outputRootPath: path.join(projectRootPath, 'output'),
      modulePath,
    };
  }

  function createModule(
    packageName: string,
    dependencies: readonly string[] = [],
    interopConfigPath?: string,
  ): InteropConfigModuleInfo {
    return {
      packageName,
      modulePath: path.join(projectRootPath, packageName),
      dependencies,
      ...(interopConfigPath === undefined ? {} : { interopConfigPath }),
    };
  }

  function createFile(module: InteropConfigModuleInfo, relativePath: string): string {
    const filePath = path.join(module.modulePath, relativePath);
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, '', 'utf8');
    return filePath;
  }

  function createModuleTable(mainModule: MainModuleInfo, ...modules: InteropConfigModuleInfo[]): ModuleTable {
    const allModules = [mainModule, ...modules];
    return {
      modules: allModules,
      byPackage: new Map(allModules.map((module) => [module.packageName, module])),
      mainModule,
    };
  }

  function writeConfig(packageName: string, content: string): string {
    const configPath = path.join(projectRootPath, packageName, 'interop-config.json5');
    fs.mkdirSync(path.dirname(configPath), { recursive: true });
    fs.writeFileSync(configPath, content, 'utf8');
    return configPath;
  }

  function captureError(table: ModuleTable, host?: Partial<InteropConfigHost>): unknown {
    try {
      resolve(table, host);
      return undefined;
    } catch (error) {
      return error;
    }
  }

  function expectConfigError(
    table: ModuleTable,
    expected: { readonly description: string; readonly position: string; readonly cause?: string },
    host?: Partial<InteropConfigHost>,
  ): void {
    const caught = captureError(table, host);
    expect(caught).toBeInstanceOf(InteropConfigError);
    expect(caught).toMatchObject({
      errorMessage: {
        description: expect.stringContaining(expected.description),
        position: expected.position,
        ...(expected.cause === undefined ? {} : { cause: expect.stringContaining(expected.cause) }),
      },
    });
  }
});
