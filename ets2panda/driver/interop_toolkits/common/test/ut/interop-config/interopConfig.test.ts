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

import { resolveInteropConfig as resolve } from '../../../src/interop-config/resolve';
import type { InteropConfigModuleInfo, MainModuleInfo, ModuleTable } from '../../../src/interop-config/types';

const FIXTURE_CONFIG_PATH = path.join(__dirname, 'interop-config.json5');
const MERGE_MAIN_CONFIG_PATH = path.join(__dirname, 'merge-main.json5');
const MERGE_CONTRIBUTOR_CONFIG_PATH = path.join(__dirname, 'merge-contributor.json5');
const PACKAGE_PRIORITY_CONFIG_PATH = path.join(__dirname, 'package-priority.json5');
const UNREACHABLE_PACKAGE_CONFIG_PATH = path.join(__dirname, 'unreachable-package.json5');

describe('resolve interop configuration', () => {
  let projectRootPath: string;

  beforeEach(() => {
    projectRootPath = fs.mkdtempSync(path.join(os.tmpdir(), 'common-interop-config-'));
  });

  afterEach(() => {
    fs.rmSync(projectRootPath, { recursive: true, force: true });
  });

  it('resolves static and dynamic entries for modules and selected dependencies', async () => {
    const mainModule = createMainModule('main-package', ['package-full', 'package-partial'], FIXTURE_CONFIG_PATH);
    const fullModule = createModule('package-full');
    const partialModule = createModule('package-partial');
    const mainStaticFile = createFile(mainModule, 'src/main.ets');
    const mainDynamicFile = createFile(mainModule, 'src/main.ts');
    const partialStaticFile = createFile(partialModule, 'src/partial.ets');
    const partialDynamicFile = createFile(partialModule, 'src/partial.ts');

    const targets = await resolve(createModuleTable(mainModule, fullModule, partialModule));

    expect(targets.get(mainModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [mainStaticFile],
      dynamicFiles: [mainDynamicFile],
    });
    expect(targets.get(partialModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [partialStaticFile],
      dynamicFiles: [partialDynamicFile],
    });
    expect(targets.get(fullModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: fullModule,
    });
  });

  it('merges static and dynamic item contributions for the same package', async () => {
    const mainModule = createMainModule('main-package', ['package-contributor'], MERGE_MAIN_CONFIG_PATH);
    const contributorModule = createModule('package-contributor', ['package-target'], MERGE_CONTRIBUTOR_CONFIG_PATH);
    const targetModule = createModule('package-target');
    const sharedStaticFile = createFile(targetModule, 'src/shared.ets');
    const mainStaticFile = createFile(targetModule, 'src/from-main.ets');
    const contributorStaticFile = createFile(targetModule, 'src/from-contributor.ets');
    const sharedDynamicFile = createFile(targetModule, 'src/shared.ts');
    const mainDynamicFile = createFile(targetModule, 'src/from-main.ts');
    const contributorDynamicFile = createFile(targetModule, 'src/from-contributor.ts');

    const targets = await resolve(createModuleTable(mainModule, contributorModule, targetModule));

    expect(targets.get(targetModule.packageName)).toEqual({
      kind: 'items',
      staticFiles: [sharedStaticFile, mainStaticFile, contributorStaticFile],
      dynamicFiles: [sharedDynamicFile, mainDynamicFile, contributorDynamicFile],
    });
  });

  it('prefers a package target over item contributions for the same package', async () => {
    const mainModule = createMainModule('main-package', ['package-priority'], MERGE_MAIN_CONFIG_PATH);
    const priorityModule = createModule('package-priority', ['package-target'], PACKAGE_PRIORITY_CONFIG_PATH);
    const targetModule = createModule('package-target');
    createFile(targetModule, 'src/shared.ets');
    createFile(targetModule, 'src/from-main.ets');
    createFile(targetModule, 'src/shared.ts');
    createFile(targetModule, 'src/from-main.ts');

    const targets = await resolve(createModuleTable(mainModule, priorityModule, targetModule));

    expect(targets.get(targetModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: targetModule,
    });
  });

  it('resolves a dependency package that is present in the module table but not reachable from main', async () => {
    const mainModule = createMainModule('main-package', [], UNREACHABLE_PACKAGE_CONFIG_PATH);
    const unreachableModule = createModule('package-unreachable');

    const targets = await resolve(createModuleTable(mainModule, unreachableModule));

    expect(targets.get(unreachableModule.packageName)).toEqual({
      kind: 'package',
      moduleInfo: unreachableModule,
    });
  });

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
});
