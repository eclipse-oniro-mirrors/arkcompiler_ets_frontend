/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
import type { BuildConfig } from '../../../src/types';
import { JobContentType } from '../../../src/types';
import { Graph, GraphNode } from '../../../src/util/graph';
import { BaseMode } from '../../../src/build/base_mode';

// State shared with the mocked TaskManager so each test can control the result
// of finish() and inspect the graph handed to it.
const mockTaskManagerState: { finishResult: boolean; capturedGraph: any } = {
    finishResult: true,
    capturedGraph: undefined
};

jest.mock('fs');

jest.mock('../../../src/logger', () => {
    const actual = jest.requireActual('../../../src/logger');
    const loggerInstance = {
        printDebug: jest.fn(),
        printInfo: jest.fn(),
        printWarn: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn()
    };
    return {
        Logger: {
            getInstance: jest.fn(() => loggerInstance)
        },
        LogDataFactory: actual.LogDataFactory
    };
});

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn(() => ({
        arkts: {},
        arktsGlobal: {}
    }))
}));

jest.mock('../../../src/util/utils', () => ({
    ...jest.requireActual('../../../src/util/utils'),
    ensurePathExists: jest.fn(),
    safeRealpath: jest.fn((p: string) => p)
}));

jest.mock('../../../src/util/ets2panda', () => ({
    Ets2panda: {
        getInstance: jest.fn(() => ({
            initalize: jest.fn(),
            compile: jest.fn(),
            declgenV1: jest.fn(),
            finalize: jest.fn()
        })),
        destroyInstance: jest.fn()
    }
}));

// Mock only the process/worker orchestration. Graph stays real so that the
// edge-pruning fix is exercised against the actual Graph.verify() logic.
jest.mock('../../../src/util/TaskManager', () => ({
    DriverProcessFactory: jest.fn().mockImplementation(() => ({})),
    TaskManager: jest.fn().mockImplementation(() => ({
        startWorkers: jest.fn(),
        initTaskQueue: jest.fn(),
        finish: jest.fn(async () => mockTaskManagerState.finishResult),
        set buildGraph(g: any) {
            mockTaskManagerState.capturedGraph = g;
        },
        get buildGraph() {
            return mockTaskManagerState.capturedGraph;
        }
    }))
}));

class TestBaseMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }
}

const FILE_A = '/mock/module/a.ets';
const FILE_B = '/mock/module/b.ets';

function makeJobData(input: string): any {
    return {
        contentType: JobContentType.FILE,
        content: { input, output: '' },
        moduleName: 'testPackage',
        moduleRoot: '/mock/module'
    };
}

/**
 * Build a two-node dependency graph where A depends on B
 * (B is a predecessor of A, A is a descendant of B).
 */
function buildTwoNodeGraph(): Graph<any> {
    const nodeA = new GraphNode<any>('A', makeJobData(FILE_A));
    const nodeB = new GraphNode<any>('B', makeJobData(FILE_B));
    nodeA.predecessors = new Set(['B']);
    nodeB.descendants = new Set(['A']);
    return Graph.createGraphFromNodes([nodeA, nodeB]);
}

function buildSingleNodeGraph(): Graph<any> {
    const nodeA = new GraphNode<any>('A', makeJobData(FILE_A));
    return Graph.createGraphFromNodes([nodeA]);
}

function createMockBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
    return {
        pandaSdkPath: '/mock/panda/sdk',
        buildSdkPath: '/mock/build/sdk',
        cachePath: '/mock/cache',
        projectRootPath: '/mock/project',
        loaderOutPath: '/mock/output',
        compileFiles: [],
        aliasConfig: {},
        interopSDKPaths: new Set(),
        externalApiPaths: [],
        packageName: 'testPackage',
        moduleRootPath: '/mock/module',
        sourceRoots: ['src'],
        dependencyModuleList: [],
        plugins: {},
        paths: {},
        ...overrides
    } as BuildConfig;
}

describe('BaseMode.generateDeclarationV1Parallel', () => {
    let mode: TestBaseMode;
    let backupSpy: jest.SpyInstance;
    let updateSpy: jest.SpyInstance;
    let saveSpy: jest.SpyInstance;

    beforeEach(() => {
        jest.clearAllMocks();
        mockTaskManagerState.finishResult = true;
        mockTaskManagerState.capturedGraph = undefined;

        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('{}');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: Date.now() });

        mode = new TestBaseMode(createMockBuildConfig());

        // Isolate the change under test from unrelated disk side effects.
        backupSpy = jest.spyOn(mode as any, 'backupDeclgenFiles').mockResolvedValue(undefined);
        updateSpy = jest.spyOn(mode as any, 'updateDeclFileMapForJobs').mockResolvedValue(undefined);
        saveSpy = jest.spyOn(mode as any, 'saveDeclFileMap').mockResolvedValue(undefined);
    });

    // Fix #1: dangling edges to filtered-out nodes must be pruned so that
    // Graph.createGraphFromNodes(...) does not fail in verify().
    test('prunes edges pointing to nodes that are not regenerated (no verify() crash)', async () => {
        const inputGraph = buildTwoNodeGraph();
        jest.spyOn(mode as any, 'getDepAnalyzer').mockReturnValue({ getGraph: () => inputGraph });
        // Only A needs regeneration; B (a predecessor of A) is up to date.
        jest.spyOn(mode as any, 'needsRegeneration').mockImplementation((...args: unknown[]) => args[0] === FILE_A);

        await expect(mode.generateDeclarationV1Parallel()).resolves.toBeUndefined();

        const captured = mockTaskManagerState.capturedGraph as Graph<any>;
        expect(captured).toBeDefined();
        // Only the regenerated node survives...
        expect(captured.nodes.size).toBe(1);
        const keptNode = captured.getNodeById('A');
        expect(keptNode).toBeDefined();
        // ...and its edge to the dropped node B has been pruned.
        expect(keptNode.predecessors.size).toBe(0);
        expect(keptNode.descendants.size).toBe(0);
        expect(captured.getNodeById('B')).toBeUndefined();

        // The original input graph must remain untouched (new Set instances used).
        expect(inputGraph.getNodeById('A').predecessors.has('B')).toBe(true);
    });

    // Regression guard documenting WHY the pruning is required: an unpruned
    // dangling edge makes Graph.createGraphFromNodes throw.
    test('Graph.createGraphFromNodes throws on a dangling edge (rationale for pruning)', () => {
        const dangling = new GraphNode<any>('A', makeJobData(FILE_A));
        dangling.predecessors = new Set(['B']); // 'B' is not part of the node list
        expect(() => Graph.createGraphFromNodes([dangling])).toThrow();
    });

    // Fix #4: on success the decl file map is persisted.
    test('persists the decl file map when all declgen jobs succeed', async () => {
        mockTaskManagerState.finishResult = true;
        jest.spyOn(mode as any, 'getDepAnalyzer').mockReturnValue({ getGraph: () => buildSingleNodeGraph() });
        jest.spyOn(mode as any, 'needsRegeneration').mockReturnValue(true);

        await expect(mode.generateDeclarationV1Parallel()).resolves.toBeUndefined();

        expect(backupSpy).toHaveBeenCalled();
        expect(updateSpy).toHaveBeenCalled();
        expect(saveSpy).toHaveBeenCalled();
    });
});
