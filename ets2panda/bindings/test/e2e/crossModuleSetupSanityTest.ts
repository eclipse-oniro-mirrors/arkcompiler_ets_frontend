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

import path from 'path';

import { ModuleDescriptor, PathConfig } from '../../src';
import { generateBuildConfigs } from '../../src/lsp/generateBuildConfig';
import { DEFAULT_PATH_CONFIG, getMultiModuleLsp, getRealPath } from '../utils';

function defineCrossModuleSanityTest(
  suiteName: string,
  projectName: string,
  entryRelativePath: string,
  harRelativePath: string
): void {
  const modules: ModuleDescriptor[] = [
    { name: 'entry', moduleType: 'har', srcPath: 'entry' },
    { name: 'har', moduleType: 'har', srcPath: 'har' }
  ];
  const projectPath = path.resolve('test/testcases/', projectName);
  const pathConfig: PathConfig = {
    ...DEFAULT_PATH_CONFIG,
    projectPath
  };
  const lsp = getMultiModuleLsp(projectName, modules, []);
  const entryFile = getRealPath(projectName, entryRelativePath);
  const harIndexFile = getRealPath(projectName, harRelativePath);

  describe(suiteName, () => {
    test('cross_module_build_and_runtime_sanity', () => {
      const resolvedModules = modules.map((module) => ({
        ...module,
        srcPath: path.resolve(projectPath, module.srcPath)
      }));
      const buildConfigs = generateBuildConfigs(pathConfig, resolvedModules, []);
      const runtimeLsp = lsp as any;
      const mergedCompileFiles = runtimeLsp.getMergedCompileFilesCrossModule(entryFile) as string[];
      const moduleInfos = runtimeLsp.moduleInfos as Record<string, unknown>;

      expect(buildConfigs['entry']?.dependencies).toStrictEqual(['har']);
      expect(mergedCompileFiles).toContain(entryFile);
      expect(mergedCompileFiles).toContain(harIndexFile);
      expect(Object.keys(moduleInfos ?? {}).length).toBeGreaterThan(0);
    });
  });
}

defineCrossModuleSanityTest(
  'crossModuleSetupSanityTest_getReferencesAtPositionCrossModule',
  'getReferencesAtPositionCrossModule',
  'entry/EntryReferences1.ets',
  'har/Index.ets'
);
