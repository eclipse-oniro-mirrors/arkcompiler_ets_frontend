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

import * as fs from 'fs';
import * as path from 'path';
import { DepAnalyzer } from '../../../src/dep_analyzer/dep_analyzer';
import { FullDepAnalyzer } from '../../../src/dep_analyzer/full_dep_analyzer';
import { Logger } from '../../../src/logger';
import { Graph } from '../../../src/util/graph';
import {
    BuildConfig, ModuleInfo, CompileJobInfo, CompileJobType,
    JobContentType, FileInfo, OHOS_MODULE_TYPE
} from '../../../src/types';

jest.mock('fs');
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

jest.mock('../../../src/build/generate_arktsconfig', () => ({
    ArkTSConfigGenerator: jest.fn().mockImplementation(() => ({
        getArktsConfigByPackageName: jest.fn()
    })),
    ArkTSConfig: jest.fn()
}));

jest.mock('../../../src/pre_define', () => {
    const actual = jest.requireActual('../../../src/pre_define');
    return { ...actual, ENABLE_DECL_FILE_CACHE: true, DECL_ETS_SUFFIX: '.d.ets' };
});

const MODULE_ROOT = '/module';
const DECLGEN_OUT_DIR = '/mock/declgen';

function createMockBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
    return {
        cachePath: '/mock/cache',
        compileFiles: ['/mock/file1.ets', '/mock/file2.ets'],
        dependencyAnalyzerPath: '/mock/bin/dependency_analyzer',
        declgenV2OutPath: DECLGEN_OUT_DIR,
        moduleType: OHOS_MODULE_TYPE.HAR,
        recordType: 'OFF' as any, buildType: 'build' as any,
        buildMode: 'Debug' as any, es2pandaMode: 'parallel' as any,
        hasMainModule: true, dependencyModuleList: [],
        ...overrides
    } as BuildConfig;
}

function createMockModuleInfo(moduleRootPath = MODULE_ROOT): ModuleInfo {
    return {
        packageName: 'testModule', moduleRootPath, sourceRoots: ['src'],
        arktsConfigFile: '/test/arktsconfig.json', isMainModule: true,
        moduleType: 'har', entryFile: `${moduleRootPath}/Entry.ets`,
        dependencies: [], staticDependencyModules: new Map(),
        dynamicDependencyModules: new Map(), staticFiles: []
    } as any;
}

function createFileNode(id: string, input: string, output: string, predecessors: Set<string> = new Set()) {
    return {
        id, predecessors, descendants: new Set<string>(),
        data: {
            contentType: JobContentType.FILE,
            content: { input, output } as FileInfo,
            jobType: CompileJobType.NONE,
            arktsConfig: '/mock/arktsconfig.json',
            moduleName: 'testModule', moduleRoot: MODULE_ROOT
        } as CompileJobInfo
    };
}

function createClusterNode(id: string, files: FileInfo[]) {
    return {
        id, predecessors: new Set<string>(), descendants: new Set<string>(),
        data: {
            contentType: JobContentType.CLUSTER, content: files,
            jobType: CompileJobType.NONE,
            arktsConfig: '/mock/arktsconfig.json',
            moduleName: 'testModule', moduleRoot: MODULE_ROOT
        } as CompileJobInfo
    };
}

function createFileToModule(files: string[], moduleRoot = '/src') {
    const info = createMockModuleInfo(moduleRoot);
    return new Map(files.map(f => [f, info]));
}

function createAnalyzer(overrides: Partial<BuildConfig> = {}) {
    const config = createMockBuildConfig(overrides);
    const Gen = (require('../../../src/build/generate_arktsconfig') as any).ArkTSConfigGenerator;
    return new FullDepAnalyzer(config, new Gen(config), false);
}

let analyzer: DepAnalyzer;

beforeEach(() => {
    jest.clearAllMocks();
    (Logger.getInstance as jest.Mock).mockReturnValue({
        printDebug: jest.fn(), printWarn: jest.fn(), printError: jest.fn(), printInfo: jest.fn()
    });
    (fs.existsSync as jest.Mock).mockReturnValue(false);
    (fs.readdirSync as jest.Mock).mockReturnValue([]);
    (fs.readFileSync as jest.Mock).mockReturnValue('{}');
    (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: 100 });
    mockUpdateFileHash.mockReturnValue(false);
    mockShouldBeUpdated.mockReturnValue(false);
    mockComputeHash.mockReturnValue('h');
    mockIsHarOrHsp.mockReturnValue(false);
    mockChangeFileExtension.mockImplementation((f: string, ext: string) => f.replace(/\.[^/.]+$/, ext));
    analyzer = createAnalyzer();
});

describe('filterGraph', () => {
    test('removes unchanged FILE node', () => {
        const node = createFileNode('f1', '/src/a.ets', '/out/a.abc');
        const graph = Graph.createGraphFromNodes([node]);
        // @ts-ignore
        analyzer['filterGraph'](graph, createFileToModule(['/src/a.ets']));
        expect(graph.nodes.size).toBe(0);
    });

    test('keeps FILE node when hash changed', () => {
        mockUpdateFileHash.mockReturnValue(true);
        const node = createFileNode('f1', '/src/a.ets', '/out/a.abc');
        const graph = Graph.createGraphFromNodes([node]);
        // @ts-ignore
        analyzer['filterGraph'](graph, createFileToModule(['/src/a.ets']));
        expect(graph.nodes.size).toBe(1);
        expect(node.data.jobType & CompileJobType.ABC).not.toBe(0);
    });

    test('keeps dependent node when predecessor changed', () => {
        mockUpdateFileHash.mockImplementation((f: string) => f === '/src/a.ets');
        const nodeA = createFileNode('f1', '/src/a.ets', '/out/a.abc');
        const nodeB = createFileNode('f2', '/src/b.ets', '/out/b.abc', new Set(['f1']));
        nodeA.descendants.add('f2');
        const graph = Graph.createGraphFromNodes([nodeA, nodeB]);
        // @ts-ignore
        analyzer['filterGraph'](graph, createFileToModule(['/src/a.ets', '/src/b.ets']));
        expect(graph.nodes.has(nodeA)).toBe(true);
        expect(graph.nodes.has(nodeB)).toBe(true);
    });

    test('removes unchanged CLUSTER node', () => {
        const node = createClusterNode('c1', [
            { input: '/src/a.ets', output: '/out/a.abc' },
            { input: '/src/b.ets', output: '/out/b.abc' }
        ]);
        const graph = Graph.createGraphFromNodes([node]);
        // @ts-ignore
        analyzer['filterGraph'](graph, createFileToModule(['/src/a.ets', '/src/b.ets']));
        expect(graph.nodes.size).toBe(0);
    });

    test('keeps CLUSTER node when some file changed', () => {
        mockUpdateFileHash.mockImplementation((f: string) => f === '/src/a.ets');
        const node = createClusterNode('c1', [
            { input: '/src/a.ets', output: '/out/a.abc' },
            { input: '/src/b.ets', output: '/out/b.abc' }
        ]);
        const graph = Graph.createGraphFromNodes([node]);
        // @ts-ignore
        analyzer['filterGraph'](graph, createFileToModule(['/src/a.ets', '/src/b.ets']));
        expect(graph.nodes.size).toBe(1);
        expect(node.data.jobType & CompileJobType.ABC).not.toBe(0);
    });
});

describe('checkClusterFilesChanged - DECL path', () => {
    const fileToModule = new Map([['/module/src/main/ets/foo.ets', createMockModuleInfo()]]);

    beforeEach(() => {
        mockIsHarOrHsp.mockReturnValue(true);
        analyzer = createAnalyzer();
        // @ts-ignore
        analyzer['declgenV2OutDir'] = DECLGEN_OUT_DIR;
        // @ts-ignore
        analyzer['mainModuleType'] = OHOS_MODULE_TYPE.HAR;
    });

    const singleFile: FileInfo[] = [{ input: '/module/src/main/ets/foo.ets', output: '/out/foo.abc' }];
    const expectedDeclPath = path.resolve(DECLGEN_OUT_DIR, 'src/main/ets/foo.d.ets');

    test('constructs correct DECL output path', () => {
        mockShouldBeUpdated.mockImplementation((_: string, t: string) => t.endsWith('.d.ets'));
        // @ts-ignore
        const result = analyzer['checkClusterFilesChanged'](singleFile, fileToModule);
        expect(mockShouldBeUpdated).toHaveBeenCalledWith('/module/src/main/ets/foo.ets', expectedDeclPath);
        expect(result & CompileJobType.DECL).not.toBe(0);
    });

    test('ABC and DECL both set when hash changed', () => {
        mockUpdateFileHash.mockReturnValue(true);
        // @ts-ignore
        const result = analyzer['checkClusterFilesChanged'](singleFile, fileToModule);
        expect(result).toBe(CompileJobType.ABC | CompileJobType.DECL);
    });

    test('only DECL set when only DECL is stale', () => {
        mockShouldBeUpdated.mockImplementation((_: string, t: string) => t.endsWith('.d.ets'));
        // @ts-ignore
        const result = analyzer['checkClusterFilesChanged'](singleFile, fileToModule);
        expect(result).toBe(CompileJobType.DECL);
        expect(result & CompileJobType.ABC).toBe(0);
    });

    test('nested directory preserves full path in DECL output', () => {
        const nested = '/module/src/main/ets/sub/dir/deep/File.ets';
        const files: FileInfo[] = [{ input: nested, output: '/out/File.abc' }];
        const ftm = new Map([[nested, createMockModuleInfo()]]);
        mockShouldBeUpdated.mockImplementation((_: string, t: string) => t.endsWith('.d.ets'));
        // @ts-ignore
        analyzer['checkClusterFilesChanged'](files, ftm);
        expect(mockShouldBeUpdated).toHaveBeenCalledWith(
            nested, `${DECLGEN_OUT_DIR}/src/main/ets/sub/dir/deep/File.d.ets`
        );
    });

    test('does not generate DECL for non-HAR/HSP module', () => {
        mockIsHarOrHsp.mockReturnValue(false);
        analyzer = createAnalyzer();
        // @ts-ignore
        analyzer['mainModuleType'] = OHOS_MODULE_TYPE.ENTRY;
        // @ts-ignore
        const result = analyzer['checkClusterFilesChanged'](singleFile, new Map());
        expect(result & CompileJobType.DECL).toBe(0);
    });
});

describe('updateNodeHashes', () => {
    test('FILE node', () => {
        // @ts-ignore
        analyzer['updateNodeHashes'](createFileNode('f1', '/src/a.ets', '/out/a.abc'));
        expect(mockUpdateFileHash).toHaveBeenCalledWith('/src/a.ets', expect.any(Object));
        expect(mockUpdateFileHash).toHaveBeenCalledTimes(1);
    });

    test('CLUSTER node', () => {
        const node = createClusterNode('c1', [
            { input: '/src/a.ets', output: '/out/a.abc' },
            { input: '/src/b.ets', output: '/out/b.abc' },
            { input: '/src/c.ets', output: '/out/c.abc' }
        ]);
        // @ts-ignore
        analyzer['updateNodeHashes'](node);
        expect(mockUpdateFileHash).toHaveBeenCalledTimes(3);
    });
});
