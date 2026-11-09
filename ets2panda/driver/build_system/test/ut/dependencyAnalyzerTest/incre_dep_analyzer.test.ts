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

import * as fs from 'fs';
import * as path from 'path';
import { DepAnalyzer } from '../../../src/dep_analyzer/dep_analyzer';
import { FullDepAnalyzer } from '../../../src/dep_analyzer/full_dep_analyzer';
import { IncreDepAnalyzer } from '../../../src/dep_analyzer/incre_dep_analyzer';
import { Logger } from '../../../src/logger';
import { Graph } from '../../../src/util/graph';
import {
    BuildConfig, ModuleInfo, CompileJobInfo, CompileJobType,
    JobContentType, FileInfo, OHOS_MODULE_TYPE
} from '../../../src/types';
import { DEP_ANALYZER_DIR, DEP_ANALYZER_OUTPUT_FILE, INCRE_DEP_ANALYZER_INPUT_FILE } from '../../../src/pre_define';

jest.mock('fs');
jest.mock('child_process', () => ({
    execSync: jest.fn()
}));
jest.mock('../../../src/logger');
jest.mock('../../../src/util/statsRecorder', () => ({
    StatisticsRecorder: jest.fn().mockImplementation(() => ({ record: jest.fn() })),
    BS_PERF_DIR: 'perf',
    BS_PERF_FILE_NAME: 'bs_record_perf.csv',
    RecordEvent: {}
}));
jest.mock('../../../src/util/dotGraphDump', () => ({ dotGraphDump: jest.fn() }));
jest.mock('lodash.clonedeep', () => ({
    __esModule: true,
    default: jest.fn((obj: any) => JSON.parse(JSON.stringify(obj)))
}));

const mockUpdateFileHash = jest.fn();
const mockShouldBeUpdated = jest.fn();
const mockEnsureDirExists = jest.fn();
const mockComputeHash = jest.fn();
const mockChangeFileExtension = jest.fn();
const mockIsHarOrHsp = jest.fn();

jest.mock('../../../src/util/utils', () => ({
    updateFileHash: (...args: any[]) => mockUpdateFileHash(...args),
    shouldBeUpdated: (...args: any[]) => mockShouldBeUpdated(...args),
    ensureDirExists: (...args: any[]) => mockEnsureDirExists(...args),
    computeHash: (...args: any[]) => mockComputeHash(...args),
    changeFileExtension: (...args: any[]) => mockChangeFileExtension(...args),
    isHarOrHsp: (...args: any[]) => mockIsHarOrHsp(...args),
    isMac: jest.fn(() => false),
    ensurePathExists: jest.fn()
}));

// Per-module arktsconfigs returned by the mocked generator
const generatorConfigs: Record<string, any> = {};
jest.mock('../../../src/build/generate_arktsconfig', () => ({
    ArkTSConfigGenerator: jest.fn().mockImplementation(() => ({
        getArktsConfigByPackageName: (name: string) => generatorConfigs[name]
    })),
    ArkTSConfig: jest.fn()
}));

jest.mock('../../../src/pre_define', () => {
    const actual = jest.requireActual('../../../src/pre_define');
    return { ...actual, ENABLE_DECL_FILE_CACHE: true, DECL_ETS_SUFFIX: '.d.ets' };
});

const MODULE_ROOT = '/module';
const CACHE_DIR = '/mock/cache';
const DEP_DIR = path.join(CACHE_DIR, DEP_ANALYZER_DIR);
const INCRE_INPUT_FILE = path.join(DEP_DIR, INCRE_DEP_ANALYZER_INPUT_FILE);
const DEP_FILE = path.join(DEP_DIR, DEP_ANALYZER_OUTPUT_FILE);

const ENTRY_A = '/src/a.ets';
const ENTRY_B = '/src/b.ets';
const DYNAMIC_SOURCE = '/lib/index.ets';
const DYNAMIC_DECL = '/lib/index.d.ets';
const IMPORTER_ABC = '/out/a.abc';

function createMockBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
    return {
        cachePath: CACHE_DIR,
        compileFiles: [ENTRY_A, ENTRY_B],
        dependencyAnalyzerPath: '/mock/bin/dependency_analyzer',
        declgenV2OutPath: '/mock/declgen',
        moduleType: OHOS_MODULE_TYPE.ENTRY,
        recordType: 'OFF' as any, buildType: 'build' as any,
        buildMode: 'Debug' as any, es2pandaMode: 'parallel' as any,
        hasMainModule: true, dependencyModuleList: [],
        ...overrides
    } as BuildConfig;
}

function createMockModuleInfo(packageName: string, isMainModule = false): ModuleInfo {
    return {
        packageName, isMainModule, moduleRootPath: MODULE_ROOT, sourceRoots: ['src'],
        arktsConfigFile: `/test/${packageName}/arktsconfig.json`,
        moduleType: 'entry', entryFile: `${MODULE_ROOT}/Entry.ets`,
        dependencies: [], staticDependencyModules: new Map(),
        dynamicDependencyModules: new Map(), staticFiles: []
    } as any;
}

function setDynamicDependencyConfig(): void {
    generatorConfigs['testModule'] = {
        dependencies: {
            'lib': { language: 'js', path: DYNAMIC_DECL, sourceFilePath: DYNAMIC_SOURCE, ohmUrl: 'lib' },
            'lib/index': { language: 'js', path: DYNAMIC_DECL, sourceFilePath: DYNAMIC_SOURCE, ohmUrl: 'lib/index' },
            'std/core': { language: 'ets', path: '/mock/etsstdlib.abc', ohmUrl: 'std/core' }
        },
        mergeArktsConfig: jest.fn(),
        object: { compilerOptions: { dependencies: {} } }
    };
}

function createAnalyzer(): IncreDepAnalyzer {
    const config = createMockBuildConfig();
    const Gen = (require('../../../src/build/generate_arktsconfig') as any).ArkTSConfigGenerator;
    return new IncreDepAnalyzer(config, new Gen(config), false);
}

let analyzer: IncreDepAnalyzer;

beforeEach(() => {
    jest.clearAllMocks();
    for (const key of Object.keys(generatorConfigs)) {
        delete generatorConfigs[key];
    }
    (Logger.getInstance as jest.Mock).mockReturnValue({
        printDebug: jest.fn(), printWarn: jest.fn(), printError: jest.fn(), printInfo: jest.fn()
    });
    (fs.existsSync as jest.Mock).mockImplementation((p: string) => p === DYNAMIC_SOURCE || p === DYNAMIC_DECL);
    (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
        if (p === DEP_FILE) {
            return JSON.stringify({
                dependencies: { [ENTRY_A]: ['lib'] },
                dependants: { 'lib': [ENTRY_A] },
                outputMatching: { [ENTRY_A]: IMPORTER_ABC }
            });
        }
        if (p === path.join(DEP_DIR, 'incre_dependency.json')) {
            return JSON.stringify({ dependencies: {}, dependants: {}, outputMatching: {} });
        }
        return '{}';
    });
    (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: 100 });
    mockUpdateFileHash.mockReturnValue(false);
    mockShouldBeUpdated.mockReturnValue(false);
    mockComputeHash.mockReturnValue('h');
    mockIsHarOrHsp.mockReturnValue(false);
    mockChangeFileExtension.mockImplementation((f: string, ext: string) => f.replace(/\.[^/.]+$/, ext));
    setDynamicDependencyConfig();
    analyzer = createAnalyzer();
});

describe('collectDynamicDependencyInputs', () => {
    test('collects dynamic source and decl for js dependencies only', () => {
        const modules = [createMockModuleInfo('testModule', true)];
        // @ts-ignore protected access
        const keyToInputs: Map<string, string[]> = analyzer['collectDynamicDependencyInputs'](modules);
        expect(keyToInputs.get('lib')).toEqual([DYNAMIC_SOURCE, DYNAMIC_DECL]);
        expect(keyToInputs.get('lib/index')).toEqual([DYNAMIC_SOURCE, DYNAMIC_DECL]);
        // 'ets' dependencies (etsstdlib, bytecode har) are not watched
        expect(keyToInputs.has('std/core')).toBe(false);
    });

    test('skips inputs missing on disk', () => {
        (fs.existsSync as jest.Mock).mockImplementation((p: string) => p === DYNAMIC_SOURCE);
        const modules = [createMockModuleInfo('testModule', true)];
        // @ts-ignore protected access
        const keyToInputs: Map<string, string[]> = analyzer['collectDynamicDependencyInputs'](modules);
        expect(keyToInputs.get('lib')).toEqual([DYNAMIC_SOURCE]);
    });

    test('deduplicates inputs shared by several module configs', () => {
        generatorConfigs['depModule'] = generatorConfigs['testModule'];
        const modules = [createMockModuleInfo('testModule', true), createMockModuleInfo('depModule')];
        // @ts-ignore protected access
        const keyToInputs: Map<string, string[]> = analyzer['collectDynamicDependencyInputs'](modules);
        expect(keyToInputs.get('lib')).toEqual([DYNAMIC_SOURCE, DYNAMIC_DECL]);
    });
});

describe('findImportersAffectedByDynamicInputs', () => {
    const prevDepMap = {
        dependencies: { [ENTRY_A]: ['lib'] },
        dependants: { 'lib': [ENTRY_A, '/src/not-entry.ets'] },
        outputMatching: { [ENTRY_A]: IMPORTER_ABC, '/src/not-entry.ets': '/out/n.abc' }
    };
    const entryFiles = new Set<string>([ENTRY_A, ENTRY_B]);
    const modules = [createMockModuleInfo('testModule', true)];

    test('returns importers of changed dynamic sources and forces their recompilation flag', () => {
        mockShouldBeUpdated.mockImplementation(
            (input: string, output: string) => input === DYNAMIC_SOURCE && output === IMPORTER_ABC
        );
        // @ts-ignore private access
        const affected: string[] = analyzer['findImportersAffectedByDynamicInputs'](modules, prevDepMap, entryFiles);
        expect(affected).toEqual([ENTRY_A]);
        // @ts-ignore private access
        const status = analyzer['filesChangeStatusCache'][ENTRY_A];
        expect(status.abcOutdated).toEqual({ isSet: true, value: true });
    });

    test('returns nothing when dynamic inputs are unchanged', () => {
        // @ts-ignore private access
        const affected: string[] = analyzer['findImportersAffectedByDynamicInputs'](modules, prevDepMap, entryFiles);
        expect(affected).toEqual([]);
        // @ts-ignore private access
        expect(analyzer['filesChangeStatusCache'][ENTRY_A]).toBeUndefined();
    });

    test('skips importers that are not entry files', () => {
        mockShouldBeUpdated.mockReturnValue(true);
        // @ts-ignore private access
        const affected: string[] = analyzer['findImportersAffectedByDynamicInputs'](modules, prevDepMap, entryFiles);
        expect(affected).toEqual([ENTRY_A]);
        expect(affected).not.toContain('/src/not-entry.ets');
    });

    test('forced flag keeps an otherwise fresh node in filterGraph', () => {
        mockShouldBeUpdated.mockImplementation(
            (input: string, output: string) => input === DYNAMIC_SOURCE && output === IMPORTER_ABC
        );
        // @ts-ignore private access
        analyzer['findImportersAffectedByDynamicInputs'](modules, prevDepMap, entryFiles);

        const node = {
            id: 'f1', predecessors: new Set<string>(), descendants: new Set<string>(),
            data: {
                contentType: JobContentType.FILE,
                content: { input: ENTRY_A, output: IMPORTER_ABC } as FileInfo,
                jobType: CompileJobType.NONE,
                arktsConfig: '/mock/arktsconfig.json',
                moduleName: 'testModule', moduleRoot: MODULE_ROOT
            } as CompileJobInfo
        };
        const graph = Graph.createGraphFromNodes([node]);
        // hash unchanged, abc mtime fresh: without the forced flag the node would be dropped
        // @ts-ignore protected access
        analyzer['filterGraph'](graph, new Map());
        expect(graph.nodes.size).toBe(1);
        expect(node.data.jobType & CompileJobType.ABC).not.toBe(0);
    });
});

describe('generateDependencyMap', () => {
    const entryFiles = new Set<string>([ENTRY_A, ENTRY_B]);
    const fileToModule = new Map<string, ModuleInfo>([
        [ENTRY_A, createMockModuleInfo('testModule', true)],
        [ENTRY_B, createMockModuleInfo('testModule', true)]
    ]);
    const modules = [createMockModuleInfo('testModule', true)];

    test('writes incre inputs and persists dep map when only a dynamic source changed', () => {
        mockShouldBeUpdated.mockImplementation(
            (input: string, output: string) => input === DYNAMIC_SOURCE && output === IMPORTER_ABC
        );
        // @ts-ignore protected access
        const depMap = analyzer['generateDependencyMap'](entryFiles, modules, fileToModule);

        const writes = (fs.writeFileSync as jest.Mock).mock.calls;
        const increWrite = writes.find(([file]: [string]) => file === INCRE_INPUT_FILE);
        expect(increWrite).toBeDefined();
        expect(increWrite![1]).toBe(ENTRY_A);
        expect(writes.some(([file]: [string]) => file === DEP_FILE)).toBe(true);
        // @ts-ignore private access
        expect(analyzer['hasModified']).toBe(true);
        expect(depMap.outputMatching[ENTRY_A]).toBe(IMPORTER_ABC);
    });

    test('does not write incre inputs when nothing changed', () => {
        // @ts-ignore protected access
        analyzer['generateDependencyMap'](entryFiles, modules, fileToModule);

        const writes = (fs.writeFileSync as jest.Mock).mock.calls;
        expect(writes.some(([file]: [string]) => file === INCRE_INPUT_FILE)).toBe(false);
        expect(writes.some(([file]: [string]) => file === DEP_FILE)).toBe(false);
        // @ts-ignore private access
        expect(analyzer['hasModified']).toBe(false);
    });

    test('deduplicates entry-modified and dynamic-affected files', () => {
        // entry file A modified by hash AND affected by dynamic source change
        mockUpdateFileHash.mockImplementation((f: string) => f === ENTRY_A);
        mockShouldBeUpdated.mockImplementation(
            (input: string, output: string) => input === DYNAMIC_SOURCE && output === IMPORTER_ABC
        );
        // @ts-ignore protected access
        analyzer['generateDependencyMap'](entryFiles, modules, fileToModule);

        const writes = (fs.writeFileSync as jest.Mock).mock.calls;
        const increWrite = writes.find(([file]: [string]) => file === INCRE_INPUT_FILE);
        expect(increWrite).toBeDefined();
        expect((increWrite![1] as string).split('\n')).toEqual([ENTRY_A]);
    });
});
