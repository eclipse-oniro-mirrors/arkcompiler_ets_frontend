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

import { BuildConfig, ModuleInfo } from '../../src/common/types';
import { Lsp } from '../../src/lsp/lsp_helper';
import path from 'path';

function createLspForTest(graph: Map<string, Set<string>>): any {
  const lsp = Object.create(Lsp.prototype) as any;
  lsp.reverseModuleDeps = graph;
  lsp.reverseClosureCache = new Map<string, Set<string>>();
  return lsp;
}

function createBuildConfig(name: string, compileFiles: string[], depModuleCompileFiles: string[] = []): BuildConfig {
  return {
    packageName: name,
    moduleType: 'test',
    moduleRootPath: `/mock/${name}`,
    language: '',
    plugins: {},
    compileFiles,
    depModuleCompileFiles,
    buildSdkPath: '',
    projectPath: '',
    declgenOutDir: ''
  } as BuildConfig;
}

function createModuleInfo(name: string, compileFiles: string[], depModuleCompileFiles: string[] = []): ModuleInfo {
  return {
    packageName: name,
    moduleRootPath: `/mock/${name}`,
    moduleType: 'test',
    entryFile: '',
    arktsConfigFile: '',
    compileFiles,
    depModuleCompileFiles,
    declgenV1OutPath: '',
    declgenBridgeCodePath: '',
    staticDepModuleInfos: [],
    dynamicDepModuleInfos: [],
    language: ''
  } as ModuleInfo;
}

describe('Lsp reverse dependency cache', () => {
  test('collectReverseDependents caches results', () => {
    const graph = new Map<string, Set<string>>([
      ['B', new Set(['A', 'C'])],
      ['C', new Set(['D'])]
    ]);
    const lsp = createLspForTest(graph);

    const first = lsp.collectReverseDependents('B');
    expect(Array.from(first).sort()).toEqual(['A', 'C', 'D']);
    expect(lsp.reverseClosureCache.get('B')).toBe(first);

    const second = lsp.collectReverseDependents('B');
    expect(second).toBe(first);
  });

  test('invalidateReverseClosureCache clears changed module and impacted dependency cache roots', () => {
    const fileA = path.resolve('/project/A.ets');
    const fileB = path.resolve('/project/B.ets');
    const prevConfigs: Record<string, BuildConfig> = {
      A: { compileFiles: [fileA], depModuleCompileFiles: [] } as unknown as BuildConfig,
      B: { compileFiles: [fileB], depModuleCompileFiles: [] } as unknown as BuildConfig
    };
    const nextConfigs: Record<string, BuildConfig> = {
      A: { compileFiles: [fileA], depModuleCompileFiles: [] } as unknown as BuildConfig,
      B: { compileFiles: [fileB], depModuleCompileFiles: [fileA] } as unknown as BuildConfig
    };
    const lsp = createLspForTest(new Map());
    const roots = lsp.collectInvalidationRoots(prevConfigs, nextConfigs) as Set<string>;
    expect(Array.from(roots).sort()).toEqual(['A', 'B']);

    const prevGraph = new Map<string, Set<string>>([
      ['A', new Set()],
      ['B', new Set()]
    ]);
    const nextGraph = new Map<string, Set<string>>([
      ['A', new Set(['B'])],
      ['B', new Set()]
    ]);

    lsp.reverseClosureCache = new Map<string, Set<string>>([
      ['A', new Set()],
      ['B', new Set(['Y'])],
      ['X', new Set(['Y'])]
    ]);

    lsp.invalidateReverseClosureCache(roots, prevGraph, nextGraph);
    expect(lsp.reverseClosureCache.has('A')).toBe(false);
    expect(lsp.reverseClosureCache.has('B')).toBe(false);
    expect(lsp.reverseClosureCache.has('X')).toBe(true);

    lsp.reverseModuleDeps = nextGraph;
    const refreshed = lsp.collectReverseDependents('A') as Set<string>;
    expect(Array.from(refreshed).sort()).toEqual(['B']);
  });

  test('buildReverseModuleDeps derives reverse edges from compile file ownership', () => {
    const lsp = createLspForTest(new Map());
    const fileA = path.resolve('/project/A.ets');
    const fileB = path.resolve('/project/B.ets');
    const fileC = path.resolve('/project/C.ets');

    lsp.buildConfigs = {
      A: createBuildConfig('A', [fileA]),
      B: createBuildConfig('B', [fileB], [fileA]),
      C: createBuildConfig('C', [fileC], [fileB])
    };

    const reverseDeps = lsp.buildReverseModuleDeps() as Map<string, Set<string>>;
    expect(Array.from(reverseDeps.get('A') ?? []).sort()).toEqual(['B']);
    expect(Array.from(reverseDeps.get('B') ?? []).sort()).toEqual(['C']);
    expect(Array.from(reverseDeps.get('C') ?? []).sort()).toEqual([]);
  });

  test('getMergedCompileFilesCrossModule merges current and transitive dependents with de-dup', () => {
    const graph = new Map<string, Set<string>>([
      ['B', new Set(['A', 'C'])],
      ['C', new Set(['D'])]
    ]);
    const lsp = createLspForTest(graph);

    const fileB = path.resolve('/project/B.ets');
    const fileB2 = path.resolve('/project/B2.ets');
    const fileBDep = path.resolve('/project/B.dep.ets');
    const fileA = path.resolve('/project/A.ets');
    const fileC = path.resolve('/project/C.ets');
    const fileD = path.resolve('/project/D.ets');
    const shared = path.resolve('/project/shared.ets');

    lsp.moduleInfos = {
      [fileB]: createModuleInfo('B', [fileB, fileB2, shared], [fileBDep, shared])
    };
    lsp.buildConfigs = {
      A: createBuildConfig('A', [fileA, shared]),
      C: createBuildConfig('C', [fileC], [shared]),
      D: createBuildConfig('D', [fileD])
    };

    const result = lsp.getMergedCompileFilesCrossModule(fileB) as string[];
    expect(result.sort()).toEqual(
      [fileB, fileB2, fileBDep, fileA, fileC, fileD, shared].sort()
    );
    expect(lsp.reverseClosureCache.has('B')).toBe(true);
  });

  test('getMergedCompileFilesCrossModule returns empty when file not in moduleInfos', () => {
    const lsp = createLspForTest(new Map());
    lsp.moduleInfos = {};
    lsp.buildConfigs = {};
    expect(lsp.getMergedCompileFilesCrossModule('/project/missing.ets')).toEqual([]);
  });

});
