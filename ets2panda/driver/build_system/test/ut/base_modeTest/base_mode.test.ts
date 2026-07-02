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

import * as path from 'path';
import * as fs from 'fs';
import type { BuildConfig, ModuleInfo, CompileJobInfo } from '../../../src/types';
import type { Graph, GraphNode } from '../../../src/util/graph';
import { OHOS_MODULE_TYPE, JobContentType, CompileJobType } from '../../../src/types';
import { BaseMode } from '../../../src/build/base_mode';
import { ErrorCode, DriverError } from '../../../src/util/error';
import { LogDataFactory } from '../../../src/logger';

type mockGraphNodeType = GraphNode<Partial<CompileJobInfo>>
type mockGraphType = Graph<Partial<CompileJobInfo>>

// Basic mocks that must be in place before requiring BaseMode
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

// Mock Ets2panda to control compile/declgen behaviour
const mockEts = {
    initalize: jest.fn(),
    compile: jest.fn(),
    declgenV1: jest.fn(),
    finalize: jest.fn()
};
jest.mock('../../../src/util/ets2panda', () => ({
    Ets2panda: {
        getInstance: jest.fn(() => mockEts),
        destroyInstance: jest.fn()
    }
}));



class TestBaseMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }
    public runCompile(id: string, job: any, incremental: boolean) { return (this as any).compile(id, job, incremental); }
    public runDeclgenV1(job: any) { return (this as any).declgenV1(job); }
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

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        isMainModule: false,
        packageName: 'testModule',
        moduleRootPath: '/mock/module',
        moduleType: 'hap',
        sourceRoots: ['src'],
        entryFile: 'index.ets',
        arktsConfigFile: '/mock/arktsconfig.json',
        dynamicDependencyModules: new Map(),
        staticDependencyModules: new Map(),
        declgenV1OutPath: '/mock/declgen/v1',
        declgenBridgeCodePath: '/mock/declgen/bridge',
        dependencies: [],
        ...overrides
    } as ModuleInfo;
}

describe('BaseMode', () => {
    let testMode: TestBaseMode;
    beforeEach(() => {
        jest.clearAllMocks();
        // Mock fs methods to prevent file system errors
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('{}');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: Date.now() });

        const config = createMockBuildConfig();
        testMode = new TestBaseMode(config);
    });

    test('compile succeeds and calls ets2panda methods', () => {
        const job: Partial<CompileJobInfo> = {
            contentType: JobContentType.FILE,
            content: { input: '/mock/module/a.ets', output: '/mock/output/a.abc' },
            moduleName: 'testPackage',
            moduleRoot: '/mock/module',
            declgenConfig: { output: '' },
            jobType: CompileJobType.ABC
        };

        mockEts.compile.mockImplementation(() => { });

        const res = testMode.runCompile('job1', job, false);
        expect(res).toBe(true);
        expect(mockEts.initalize).toHaveBeenCalled();
        expect(mockEts.compile).toHaveBeenCalledWith('job1', job, false);
        expect(mockEts.finalize).toHaveBeenCalled();
    });

    test('compile handles DriverError and returns false', () => {
        const configHar = createMockBuildConfig({ moduleType: OHOS_MODULE_TYPE.HAR });
        const modeHar = new TestBaseMode(configHar);

        const job = {
            contentType: JobContentType.FILE,
            content: { input: '/mock/module/a.ets', output: '/mock/output/a.abc' },
            arktsConfig: '',
            moduleName: 'testPackage',
            moduleRoot: '/mock/module',
            declgenConfig: { output: '' },
            type: CompileJobType.ABC
        };

        const driverErr = new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'fail'));
        mockEts.compile.mockImplementation(() => { throw driverErr; });

        const logger = require('../../../src/logger').Logger.getInstance();
        const spy = jest.spyOn(logger, 'printError').mockImplementation(() => { });

        const res = modeHar.runCompile('job2', job, false);
        expect(res).toBe(false);
        expect(spy).toHaveBeenCalled();
        expect(mockEts.finalize).toHaveBeenCalled();
    });

    test('compile succeeds and calls ets2panda methods [cluster]', () => {
        const job: Partial<CompileJobInfo> = {
            contentType: JobContentType.CLUSTER,
            content: [
                { input: '/mock/module/a.ets', output: '' },
                { input: '/mock/module/b.ets', output: '' }
            ],
            moduleName: 'testPackage',
            moduleRoot: '/mock/module',
            declgenConfig: { output: '' },
            jobType: CompileJobType.ABC
        };

        mockEts.compile.mockImplementation(() => { });

        const res = testMode.runCompile('cycle1', job, true);
        expect(res).toBe(true);
        expect(mockEts.initalize).toHaveBeenCalled();
        expect(mockEts.compile).toHaveBeenCalledWith('cycle1', job, true);
        expect(mockEts.finalize).toHaveBeenCalled();
    });

    test('declgenV1 calls ets2panda.declgenV1 and returns true on success', () => {
        const moduleInfo = createMockModuleInfo({ packageName: 'testPackage', moduleRootPath: '/mock/module' });
        (testMode as any).fileToModule.set('/mock/module/a.ets', moduleInfo);

        const job: Partial<CompileJobInfo> = {
            contentType: JobContentType.FILE,
            content: { input: '/mock/module/a.ets', output: '' },
            moduleName: 'testPackage',
            moduleRoot: '/mock/module',
            declgenConfig: { output: '' },
            jobType: CompileJobType.DECL
        };

        mockEts.declgenV1.mockImplementation(() => { });
        const res = testMode.runDeclgenV1(job);
        expect(res).toBe(true);
        expect(mockEts.initalize).toHaveBeenCalled();
        expect(mockEts.declgenV1).toHaveBeenCalled();
        expect(mockEts.finalize).toHaveBeenCalled();
    });

    test('declgenV1 handles DriverError and returns false', () => {
        const moduleInfo = createMockModuleInfo({ packageName: 'testPackage', moduleRootPath: '/mock/module' });
        (testMode as any).fileToModule.set('/mock/module/a.ets', moduleInfo);

        const job: Partial<CompileJobInfo> = {
            contentType: JobContentType.FILE,
            content: { input: '/mock/module/a.ets', output: '' },
            moduleName: 'testPackage',
            moduleRoot: '/mock/module',
            declgenConfig: { output: '' },
            jobType: CompileJobType.DECL
        };

        const driverErr = new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_DECLGEN_FAIL, 'declgen fail'));
        mockEts.declgenV1.mockImplementation(() => { throw driverErr; });

        const logger = require('../../../src/logger').Logger.getInstance();
        const spy = jest.spyOn(logger, 'printError').mockImplementation(() => { });

        const res = testMode.runDeclgenV1(job);
        expect(res).toBe(false);
        expect(spy).toHaveBeenCalled();
        expect(mockEts.finalize).toHaveBeenCalled();
    });

    test('collectAbcFileFromByteCodeHar: HAR build skips collection', () => {
        const configHar = createMockBuildConfig({ moduleType: 'har' as any });
        const modeHar = new TestBaseMode(configHar);

        const moduleInfo = createMockModuleInfo({
            packageName: 'depHar',
            moduleType: 'har',
            byteCodeHar: true,
            abcPath: '/mock/depHar/out.abc',
        });
        (modeHar as any).moduleInfos = new Map<string, any>([['depHar', moduleInfo]]);

        (modeHar as any).collectAbcFileFromByteCodeHar();
        const abcFiles = (modeHar as any).abcFiles as Set<string>;
        expect(abcFiles.size).toBe(0);
    });

    test('collectAbcFileFromByteCodeHar: missing abcPath logs error', () => {
        const moduleInfo = createMockModuleInfo({
            packageName: 'depHar2',
            moduleType: 'har',
            byteCodeHar: true,
            abcPath: undefined,
        });
        (testMode as any).moduleInfos = new Map<string, any>([['depHar2', moduleInfo]]);

        const logger = require('../../../src/logger').Logger.getInstance();
        const spyErr = jest.spyOn(logger, 'printError').mockImplementation(() => { });

        (testMode as any).collectAbcFileFromByteCodeHar();
        expect(spyErr).toHaveBeenCalled();
        const abcFiles = (testMode as any).abcFiles as Set<string>;
        expect(abcFiles.size).toBe(0);
    });

    test('collectAbcFileFromByteCodeHar: abcPath exists is added', () => {
        const abcPath = '/mock/depHar3/out.abc';
        const moduleInfo = createMockModuleInfo({
            packageName: 'depHar3',
            moduleType: 'har',
            byteCodeHar: true,
            abcPath: abcPath,
        });
        (testMode as any).moduleInfos = new Map<string, any>([['depHar3', moduleInfo]]);

        // make fs.existsSync return true for this path
        (fs.existsSync as jest.Mock).mockImplementation((p: string) => p === abcPath);

        (testMode as any).collectAbcFileFromByteCodeHar();
        const abcFiles = (testMode as any).abcFiles as Set<string>;
        expect(abcFiles.has(abcPath)).toBe(true);
    });

    test('mergeAbcFiles writes linker input and calls execSync', () => {
        const child_process = require('child_process');
        const pre = require('../../../src/pre_define');
        const os = require('os');

        // prepare abcFiles in instance
        const abcPath = '/mock/collected/out.abc';
        (testMode as any).abcFiles = new Set<string>([abcPath]);

        const outputs = ['/mock/output1.abc'];

        // ensure fs.existsSync returns true for abcPath
        (fs.existsSync as jest.Mock).mockImplementation((p: string) => p === abcPath);

        // mock execSync
        const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));

        // set abcLinkerPath in build config
        (testMode as any).buildConfig.abcLinkerPath = '/mock/abc_linker';

        (testMode as any).mergeAbcFiles(outputs);

        const expectedLinkerInput = path.join((testMode as any).cacheDir, pre.LINKER_INPUT_FILE);
        const expectedContent = outputs.concat(Array.from((testMode as any).abcFiles)).join(os.EOL);
        expect(fs.writeFileSync).toHaveBeenCalledWith(expectedLinkerInput, expectedContent);
        expect(execSpy).toHaveBeenCalled();
        const execArg = execSpy.mock.calls[0][0] as string;
        expect(execArg).toContain('"' + (testMode as any).buildConfig.abcLinkerPath + '"');
        expect(execArg).toContain('@"' + expectedLinkerInput + '"');

        execSpy.mockRestore();
    });

    test('mergeAbcFiles logs error when execSync throws', () => {
        const child_process = require('child_process');

        (testMode as any).abcFiles = new Set<string>();
        const outputs = ['/mock/output1.abc'];
        (testMode as any).buildConfig.abcLinkerPath = '/mock/abc_linker';

        // make execSync throw
        const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => { throw new Error('fail'); });

        const logger = require('../../../src/logger').Logger.getInstance();
        const spyErr = jest.spyOn(logger, 'printError').mockImplementation(() => { });

        (testMode as any).mergeAbcFiles(outputs);

        expect(spyErr).toHaveBeenCalled();

        execSpy.mockRestore();
    });

    test('mergeAbcFiles Mac branch sets DYLD_LIBRARY_PATH and calls ensurePathExists', () => {
        jest.resetModules();

        // mock utils to force isMac = true and spy ensurePathExists
        jest.doMock('../../../src/util/utils', () => ({
            ...jest.requireActual('../../../src/util/utils'),
            ensurePathExists: jest.fn(),
            isMac: jest.fn(() => true)
        }));

        const child_process = require('child_process');

        // prepare instance — re-require BaseMode after mocking utils so the mock is applied
        const cfg = createMockBuildConfig();
        // mock ArkTSConfigGenerator so constructor won't try to resolve real SDK paths
        jest.doMock('../../../src/build/generate_arktsconfig', () => ({
            ArkTSConfigGenerator: jest.fn(() => ({
                generateArkTSConfigFile: jest.fn(),
                getArktsConfigByPackageName: jest.fn(() => ({
                    mergeArktsConfig: jest.fn(),
                    mergeArktsConfigByDependencies: jest.fn(),
                    object: {}
                }))
            }))
        }));
        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        class LocalTestMode extends BaseModeClass { constructor(c: any) { super(c); } }
        const mode = new LocalTestMode(cfg);
        const abcPath = '/mock/collected/out.abc';
        (mode as any).abcFiles = new Set([abcPath]);
        (cfg as any).abcLinkerPath = '/mock/abc_linker';

        (fs.existsSync as jest.Mock).mockImplementation((p: string) => p === abcPath);

        const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));

        (mode as any).mergeAbcFiles(['/mock/out.abc']);

        const calledArg = execSpy.mock.calls[0][0] as string;
        expect(calledArg).toContain('DYLD_LIBRARY_PATH=');
        const utils = require('../../../src/util/utils');
        expect(utils.ensurePathExists).toHaveBeenCalled();

        execSpy.mockRestore();
    });

    test('getDependencyModules classifies dependencies by language and throws when missing', () => {
        const { LANGUAGE_VERSION } = require('../../../src/pre_define');

        // prepare moduleInfos with three dependency modules
        const staticMod = createMockModuleInfo({ packageName: 'staticMod', language: LANGUAGE_VERSION.ARKTS_1_2 });
        const dynamicMod = createMockModuleInfo({ packageName: 'dynamicMod', language: LANGUAGE_VERSION.ARKTS_1_1 });
        const hybridMod = createMockModuleInfo({ packageName: 'hybridMod', language: LANGUAGE_VERSION.ARKTS_HYBRID });

        // main module depends on all three
        const mainModule = createMockModuleInfo({
            packageName: 'mainPkg',
            dependencies: ['staticMod', 'dynamicMod', 'hybridMod']
        });

        // set moduleInfos map
        (testMode as any).moduleInfos = new Map<string, any>([
            ['mainPkg', mainModule],
            ['staticMod', staticMod],
            ['dynamicMod', dynamicMod],
            ['hybridMod', hybridMod],
        ]);

        // call getDependencyModules
        const [dynamicDeps, staticDeps] = (testMode as any).getDependencyModules(mainModule) as [Map<string, any>, Map<string, any>];

        // staticMod -> static only
        expect(staticDeps.has('staticMod')).toBe(true);
        expect(dynamicDeps.has('staticMod')).toBe(false);

        // dynamicMod -> dynamic only
        expect(dynamicDeps.has('dynamicMod')).toBe(true);
        expect(staticDeps.has('dynamicMod')).toBe(false);

        // hybridMod -> both
        expect(staticDeps.has('hybridMod')).toBe(true);
        expect(dynamicDeps.has('hybridMod')).toBe(true);
    });

    test('getDependencyModules throws DriverError when dependency missing', () => {
        const mainModule = createMockModuleInfo({ packageName: 'mainMissing', dependencies: ['noSuchPkg'] });
        (testMode as any).moduleInfos = new Map<string, any>([['mainMissing', mainModule]]);

        expect(() => {
            (testMode as any).getDependencyModules(mainModule)
        }).toThrow();
    });

    test('processDependencyModule sets maps correctly for each language', () => {
        const { LANGUAGE_VERSION } = require('../../../src/pre_define');

        const staticMod = createMockModuleInfo({ packageName: 'p_static', language: LANGUAGE_VERSION.ARKTS_1_2 });
        const dynamicMod = createMockModuleInfo({ packageName: 'p_dynamic', language: LANGUAGE_VERSION.ARKTS_1_1 });
        const hybridMod = createMockModuleInfo({ packageName: 'p_hybrid', language: LANGUAGE_VERSION.ARKTS_HYBRID });

        const dynamicMap = new Map<string, any>();
        const staticMap = new Map<string, any>();

        // static -> only static
        (testMode as any).processDependencyModule('p_static', staticMod, dynamicMap, staticMap);
        expect(staticMap.has('p_static')).toBe(true);
        expect(dynamicMap.has('p_static')).toBe(false);

        // clear maps
        dynamicMap.clear(); staticMap.clear();

        // dynamic -> only dynamic
        (testMode as any).processDependencyModule('p_dynamic', dynamicMod, dynamicMap, staticMap);
        expect(dynamicMap.has('p_dynamic')).toBe(true);
        expect(staticMap.has('p_dynamic')).toBe(false);

        // clear maps
        dynamicMap.clear(); staticMap.clear();

        // hybrid -> both
        (testMode as any).processDependencyModule('p_hybrid', hybridMod, dynamicMap, staticMap);
        expect(dynamicMap.has('p_hybrid')).toBe(true);
        expect(staticMap.has('p_hybrid')).toBe(true);
    });

    test('processEntryFiles maps files to their modules', () => {
        const file = '/mock/module/src/foo.ets';
        const moduleInfo = createMockModuleInfo({ packageName: 'mod1', moduleRootPath: '/mock/module' });
        (testMode as any).moduleInfos = new Map<string, any>([['mod1', moduleInfo]]);
        (testMode as any).entryFiles = new Set<string>([file]);

        (testMode as any).processEntryFiles();

        const fileToModule = (testMode as any).fileToModule as Map<string, any>;
        expect(fileToModule.has(path.resolve(file))).toBe(true);
        expect(fileToModule.get(path.resolve(file))).toBe(moduleInfo);
    });

    test('processEntryFiles throws when file does not belong to any module', () => {
        const file = '/unknown/file.ets';
        const moduleInfo = createMockModuleInfo({ packageName: 'mod1', moduleRootPath: '/mock/module' });
        (testMode as any).moduleInfos = new Map<string, any>([['mod1', moduleInfo]]);
        (testMode as any).entryFiles = new Set<string>([file]);

        expect(() => {
            (testMode as any).processEntryFiles();
        }).toThrow(DriverError);
    });

    test('processEntryFiles filters out .d.ets when enableDeclgenEts2Ts is false', () => {
        const declSuffix = require('../../../src/pre_define').DECL_ETS_SUFFIX;
        const dFile = `/mock/module/src/a${declSuffix}`;
        const normalFile = '/mock/module/src/b.ets';
        const moduleInfo = createMockModuleInfo({ packageName: 'mod1', moduleRootPath: '/mock/module' });
        const cfg = createMockBuildConfig({ enableDeclgenEts2Ts: false });
        (testMode as any).buildConfig = cfg;

        (testMode as any).moduleInfos = new Map<string, any>([['mod1', moduleInfo]]);
        (testMode as any).entryFiles = new Set<string>([dFile, normalFile]);

        (testMode as any).processEntryFiles();

        const remaining = Array.from((testMode as any).entryFiles) as string[];
        expect(remaining).toEqual(expect.arrayContaining([normalFile]));
        expect(remaining).not.toEqual(expect.arrayContaining([dFile]));
    });

    test('runParallel calls mergeAbcFiles on success', async () => {
        jest.resetModules();
        const mockFinish = jest.fn().mockResolvedValue(true);

        jest.doMock('../../../src/util/TaskManager', () => {
            return {
                TaskManager: jest.fn().mockImplementation(() => ({
                    startWorkers: jest.fn(),
                    initTaskQueue: jest.fn(),
                    finish: mockFinish
                })),
                DriverProcessFactory: jest.fn().mockImplementation(() => ({}))
            };
        });

        jest.doMock('../../../src/build/generate_arktsconfig', () => ({
            ArkTSConfigGenerator: {
                getInstance: jest.fn(() => ({
                    generateArkTSConfigFile: jest.fn(),
                    getArktsConfigByPackageName: jest.fn(() => ({
                        mergeArktsConfig: jest.fn(),
                        mergeArktsConfigByDependencies: jest.fn(),
                        object: {}
                    }))
                }))
            }
        }));

        // require a fresh BaseMode with the above mocks
        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const runParallelFn = (BaseModeClass as any).prototype.runParallel;

        const cfg = createMockBuildConfig({ compileFiles: Array.from({ length: 1000 }, (_, i) => `f${i}`) });
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn((_1: any, _2: any, _3: any, allOutputs: any[]) => {
                        allOutputs.push('/mock/out.abc');
                        return { hasNodes: () => true, nodes: [] };
                    })
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(Array.from({ length: 1000 }, (_, i) => `f${i}`)),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: require('../../../src/logger').Logger.getInstance(),
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        ctx.mergeAbcFiles = jest.fn();

        await runParallelFn.call(ctx);

        expect(ctx.mergeAbcFiles).toHaveBeenCalledWith(['/mock/out.abc']);
    });

    test('runParallel throws when TaskManager.finish returns false', async () => {
        jest.resetModules();
        const mockFinish = jest.fn().mockResolvedValue(false);

        jest.doMock('../../../src/util/TaskManager', () => {
            return {
                TaskManager: jest.fn().mockImplementation(() => ({
                    startWorkers: jest.fn(),
                    initTaskQueue: jest.fn(),
                    finish: mockFinish
                })),
                DriverProcessFactory: jest.fn().mockImplementation(() => ({}))
            };
        });

        jest.doMock('../../../src/build/generate_arktsconfig', () => ({
            ArkTSConfigGenerator: {
                getInstance: jest.fn(() => ({
                    generateArkTSConfigFile: jest.fn(),
                    getArktsConfigByPackageName: jest.fn(() => ({
                        mergeArktsConfig: jest.fn(),
                        mergeArktsConfigByDependencies: jest.fn(),
                        object: {}
                    }))
                }))
            }
        }));

        const BaseModeModule2 = require('../../../src/build/base_mode');
        const BaseModeClass2 = BaseModeModule2.BaseMode;
        const runParallelFn2 = (BaseModeClass2 as any).prototype.runParallel;

        const cfg2 = createMockBuildConfig({ compileFiles: Array.from({ length: 1000 }, (_, i) => `f${i}`) });
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => true, nodes: [] }))
        };
        const ctx2: any = {
            buildConfig: cfg2,
            entryFiles: new Set(Array.from({ length: 1000 }, (_, i) => `f${i}`)),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: require('../../../src/logger').Logger.getInstance(),
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        await expect(runParallelFn2.call(ctx2)).rejects.toThrow('Parallel run failed.');
    });

    test('collectModuleInfos populates moduleInfos and normalizes entryFile', () => {
        const pre = require('../../../src/pre_define');
        const { LANGUAGE_VERSION } = pre;

        const dep = {
            packageName: 'depPkg',
            moduleName: 'depModule',
            modulePath: '/mock/depPkg',
            moduleType: 'hap',
            sourceRoots: ['src'],
            entryFile: '/mock/depPkg/index.ets',
            language: LANGUAGE_VERSION.ARKTS_1_2,
            dependencies: []
        };

        const newConfig = createMockBuildConfig({
            packageName: 'mainPkg',
            moduleRootPath: '/mock/main',
            sourceRoots: ['src'],
            dependencyModuleList: [dep]
        });

        // inject new buildConfig into the instance and clear existing moduleInfos
        (testMode as any).buildConfig = newConfig;
        (testMode as any).moduleInfos = new Map<string, any>();

        (testMode as any).collectModuleInfos();

        // main module should be present
        expect((testMode as any).moduleInfos.has('mainPkg')).toBe(true);
        // dependency should be added
        expect((testMode as any).moduleInfos.has('depPkg')).toBe(true);

        // main module dependencies should include depPkg
        const mainModule = (testMode as any).moduleInfos.get('mainPkg');
        expect(mainModule.dependencies.includes('depPkg')).toBe(true);

        // dependency entryFile should be normalized to relative path (index.ets)
        const depModule = (testMode as any).moduleInfos.get('depPkg');
        expect(depModule.entryFile).toBe('index.ets');
    });

    test('collectModuleInfos throws when dependency info is not correct', () => {
        const badDep = {
            packageName: '',
            modulePath: '',
            entryFile: ''
        };
        const newConfig = createMockBuildConfig({
            packageName: 'mainPkg',
            moduleRootPath: '/mock/main',
            sourceRoots: ['src'],
            dependencyModuleList: [badDep as any]
        });

        (testMode as any).buildConfig = newConfig;
        (testMode as any).moduleInfos = new Map<string, any>();

        expect(() => {
            (testMode as any).collectModuleInfos();
        }).toThrow(DriverError);
    });

    test('collectModuleInfos throws when hasMainModule set but main info incomplete', () => {
        const newConfig = createMockBuildConfig({
            hasMainModule: true as any,
            packageName: '',
            moduleRootPath: undefined as any,
            sourceRoots: undefined as any,
            dependencyModuleList: []
        });

        (testMode as any).buildConfig = newConfig;
        (testMode as any).moduleInfos = new Map<string, any>();

        expect(() => {
            (testMode as any).collectModuleInfos();
        }).toThrow(DriverError);
    });

    test('backwardCompatibilityWorkaroundStub sets mainModule.entryFile from fileToModule', () => {
        const file = '/mock/module/src/main.ets';
        const mainModule = createMockModuleInfo({ packageName: 'testPackage', isMainModule: true });

        // ensure moduleInfos contains main package and fileToModule maps resolved path to mainModule
        (testMode as any).moduleInfos.set('testPackage', mainModule);
        (testMode as any).fileToModule = new Map<string, any>([[path.resolve(file), mainModule]]);

        (testMode as any).backwardCompatibilityWorkaroundStub();

        expect(mainModule.entryFile).toBe(path.resolve(file));
    });

    test('backwardCompatibilityWorkaroundStub sets undefined when no main files present', () => {
        const mainModule = createMockModuleInfo({ packageName: 'testPackage', isMainModule: true, entryFile: 'original.ets' });

        // put a different module into fileToModule (not main)
        const otherModule = createMockModuleInfo({ packageName: 'other', isMainModule: false });
        (testMode as any).moduleInfos.set('testPackage', mainModule);
        (testMode as any).fileToModule = new Map<string, any>([[path.resolve('/some/other.ets'), otherModule]]);

        (testMode as any).backwardCompatibilityWorkaroundStub();

        expect((testMode as any).moduleInfos.get('testPackage').entryFile).toBeUndefined();
    });

    test('generateDeclarationV1Parallel returns early when no nodes', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        // mock TaskManager to ensure no workers are started when no nodes
        jest.doMock('../../../src/util/TaskManager', () => ({
            TaskManager: jest.fn().mockImplementation(() => ({
                startWorkers: jest.fn(),
                initTaskQueue: jest.fn(),
                finish: jest.fn()
            })),
            DriverProcessFactory: jest.fn().mockImplementation(() => ({}))
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateDeclarationV1Parallel;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => false }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set<string>(),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            loadDeclFileMap: jest.fn(),
            saveDeclFileMap: jest.fn(),
            needsRegeneration: jest.fn(),
            backupDeclgenFiles: jest.fn(),
            updateDeclFileMapForJobs: jest.fn(),
            declFileMap: new Map(),
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        ctx.needsRegeneration.mockReturnValue(true);
        ctx.backupDeclgenFiles.mockResolvedValue(undefined);
        ctx.updateDeclFileMapForJobs.mockResolvedValue(undefined);
        ctx.saveDeclFileMap.mockResolvedValue(undefined);

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(loggerInstance.printWarn).toHaveBeenCalledWith('Nothing to compile. Exiting...');
    });

    test('generateDeclarationV1Parallel starts workers and awaits finish on nodes', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        const mockModuleInfo = createMockModuleInfo();
        const mockFinish = jest.fn().mockResolvedValue(undefined);
        const startWorkersSpy = jest.fn();
        jest.doMock('../../../src/util/TaskManager', () => {
            return {
                TaskManager: jest.fn().mockImplementation(() => ({
                    startWorkers: startWorkersSpy,
                    initTaskQueue: jest.fn(),
                    finish: mockFinish
                })),
                DriverProcessFactory: jest.fn().mockImplementation(() => ({}))
            };
        });

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateDeclarationV1Parallel;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn((_1: any, _2: any, _3: any, _4: any[]): Partial<mockGraphType> => {
                        return {
                            hasNodes: () => true,
                            nodes: new Set<mockGraphNodeType>([{
                                id: 'n1',
                                data: {
                                    contentType: JobContentType.FILE,
                                    content: { input: '/mock/module/a.ets', output: '' },
                                    moduleName: mockModuleInfo.packageName,
                                    jobType: CompileJobType.ABC,
                                },
                                predecessors: new Set<string>,
                                descendants: new Set<string>
                            }])
                        };
                    })
        }
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map<string, Partial<ModuleInfo>>([[
                '/mock/module/a.ets',
                mockModuleInfo
            ]]),
            moduleInfos: new Map<string, ModuleInfo>([[mockModuleInfo.packageName, mockModuleInfo]]),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            loadDeclFileMap: jest.fn(),
            saveDeclFileMap: jest.fn(),
            needsRegeneration: jest.fn(),
            backupDeclgenFiles: jest.fn(),
            updateDeclFileMapForJobs: jest.fn(),
            declFileMap: new Map(),
            filterFilesNeedRegeneration: jest.fn(),
            buildJobFileToModuleMap: jest.fn(),
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        ctx.needsRegeneration.mockReturnValue(true);
        ctx.filterFilesNeedRegeneration.mockImplementation((jobInfo: any) => {
            const contentFiles = jobInfo.contentType === JobContentType.FILE
                ? [jobInfo.content]
                : jobInfo.content;
            return contentFiles.filter(() => true);
        });
        ctx.backupDeclgenFiles.mockResolvedValue(undefined);
        ctx.updateDeclFileMapForJobs.mockResolvedValue(undefined);
        ctx.saveDeclFileMap.mockResolvedValue(undefined);
        ctx.buildJobFileToModuleMap.mockReturnValue({ '/mock/module/a.ets': mockModuleInfo });

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(startWorkersSpy).toHaveBeenCalled();
        expect(mockFinish).toHaveBeenCalled();
    });

    test('generateDeclarationV1 returns early when no nodes', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        // mock ets2panda to observe getInstance/destroyInstance
        const mockEtsLocal = {};
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: jest.fn()
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateDeclarationV1;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => false }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set<string>(),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        // should not throw and should call logger.printWarn
        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(loggerInstance.printWarn).toHaveBeenCalledWith('Nothing to compile. Exiting...');
    });

    test('generateDeclarationV1 runs declgenV1 for all jobs and destroys Ets2panda', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        // Graph.topologicalSort mock
        jest.doMock('../../../src/util/graph', () => ({
            Graph: { topologicalSort: jest.fn(() => ['n1', 'n2']) }
        }));

        const mockEtsLocal = {};
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateDeclarationV1;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({
                        hasNodes: () => true,
                        getNodeById: (id: string): Partial<GraphNode<Partial<CompileJobInfo>>> => ({
                            id,
                            data: {
                                contentType: JobContentType.FILE,
                                content: [],
                                jobType: CompileJobType.NONE,
                            }
                        })
                    }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            // declgenV1 returns true for all jobs
            declgenV1: jest.fn(() => true),
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(require('../../../src/util/ets2panda').Ets2panda.getInstance).toHaveBeenCalledWith(cfg);
        expect(destroySpy).toHaveBeenCalled();
        expect((ctx.declgenV1 as jest.Mock).mock.calls.length).toBe(2);
    });

    test('generateDeclarationV1 throws DriverError when any declgenV1 fails', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        jest.doMock('../../../src/util/graph', () => ({
            Graph: { topologicalSort: jest.fn(() => ['only']) }
        }));

        const mockEtsLocal = {};
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateDeclarationV1;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({
                        hasNodes: () => true,
                        getNodeById: (id: string): Partial<GraphNode<Partial<CompileJobInfo>>> => ({
                            id,
                            data: {
                                contentType: JobContentType.FILE,
                                content: [],
                                jobType: CompileJobType.NONE,
                            }
                        })
                    }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            // declgenV1 fails
            declgenV1: jest.fn(() => false),
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        await expect(fn.call(ctx)).rejects.toThrow();
        expect(destroySpy).toHaveBeenCalled();
    });

    test('generateArkTSConfigForModules merges and writes arktsconfigs', () => {
        jest.resetModules();

        const arktsA = { mergeArktsConfig: jest.fn(), mergeArktsConfigByDependencies: jest.fn(), object: { a: 1 } };
        const arktsB = { mergeArktsConfig: jest.fn(), mergeArktsConfigByDependencies: jest.fn(), object: { b: 2 } };

        const genInstance = {
            generateArkTSConfigFile: jest.fn(),
            getArktsConfigByPackageName: jest.fn((pkg: string) => (pkg === 'B' ? arktsB : arktsA))
        };

        jest.doMock('../../../src/build/generate_arktsconfig', () => ({
            ArkTSConfigGenerator: jest.fn(() => genInstance)
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.generateArkTSConfigForModules;

        const moduleA = createMockModuleInfo({ packageName: 'A', dependencies: ['B'], arktsConfigFile: '/tmp/A.json' });
        const moduleB = createMockModuleInfo({ packageName: 'B', dependencies: [], arktsConfigFile: '/tmp/B.json' });

        const cfg = createMockBuildConfig();
        const ctx: any = {
            buildConfig: cfg,
            moduleInfos: new Map<string, any>([['A', moduleA], ['B', moduleB]]),
            enableDeclgenEts2Ts: false,
            logger: require('../../../src/logger').Logger.getInstance(),
            arktsConfigGenerator: genInstance,
            generateDependencyOhmUrlMap: jest.fn(),
        };

        fn.call(ctx);

        expect(genInstance.generateArkTSConfigFile).toHaveBeenCalledTimes(2);
        expect(genInstance.getArktsConfigByPackageName).toHaveBeenCalledWith('A');
        expect(genInstance.getArktsConfigByPackageName).toHaveBeenCalledWith('B');
        // merging: A should recurse into its dependencies
        expect(arktsA.mergeArktsConfigByDependencies).toHaveBeenCalled();
        // at least one write happened (configs flushed)
        expect(fs.writeFileSync).toHaveBeenCalled();
    });

    test('run calls compile and mergeAbcFiles on success', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        jest.doMock('../../../src/util/graph', () => ({
            Graph: { topologicalSort: jest.fn(() => ['n1']) }
        }));

        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.run;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn((_1: any, _2: any, _3: any, allOutputs: any[]): Partial<mockGraphType> => {
                        allOutputs.push('/mock/out.abc');
                        return {
                            hasNodes: () => true,
                            getNodeById: (_: string): mockGraphNodeType => ({
                                id: 'n1',
                                data: {
                                    contentType: JobContentType.FILE,
                                    content: { input: '/mock/module/a.ets', output: '/mock/output/a.abc' },
                                    moduleName: 'test',
                                    moduleRoot: '/mock/module',
                                },
                                predecessors: new Set<string>,
                                descendants: new Set<string>
                            }),
                            nodes: new Set<mockGraphNodeType>
                        };
                    })
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        // provide compile implementation used by run
        ctx.compile = jest.fn(() => true);
        ctx.mergeAbcFiles = jest.fn();

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(destroySpy).toHaveBeenCalled();
        expect(ctx.mergeAbcFiles).toHaveBeenCalledWith(['/mock/out.abc']);
    });

    test('run throws when compile returns false', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        jest.doMock('../../../src/util/graph', () => ({
            Graph: { topologicalSort: jest.fn(() => ['n1']) }
        }));

        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.run;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn((_1: any, _2: any, _3: any, allOutputs: any[]): Partial<mockGraphType> => {
                        allOutputs.push('/mock/out.abc');
                        return {
                            hasNodes: () => true,
                            getNodeById: (_: string) => ({
                                id: 'n1',
                                data: {
                                    contentType: JobContentType.FILE,
                                    content: { input: '/mock/module/a.ets', output: '/mock/output/a.abc' },
                                    moduleName: 'test',
                                    moduleRoot: '/mock/module'
                                },
                                predecessors: new Set<string>,
                                descendants: new Set<string>
                            }),
                            nodes: new Set<mockGraphNodeType>
                        };
                    })
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            // make compile return false to simulate failure
            compile: jest.fn(() => false),
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };


        const err: DriverError = new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                                                                            'One or more errors occured.'))
        await expect(fn.call(ctx)).rejects.toThrow(err);
        expect(destroySpy).toHaveBeenCalled();
    });

    test('run returns early when no nodes', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.run;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => false }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set<string>(),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(loggerInstance.printWarn).toHaveBeenCalledWith('Nothing to compile. Exiting...');
    });

    test('run calls compile when cycle job present', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        // Graph.topologicalSort should return the node id
        jest.doMock('../../../src/util/graph', () => ({ Graph: { topologicalSort: jest.fn(() => ['n1']) } }));

        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.run;

        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn((_entryFiles: any, _fileToModule: any, _moduleInfos: any, allOutputs: any[]) => {
                    allOutputs.push('/mock/out.abc');
                    return {
                        hasNodes: () => true,
                        getNodeById: ((_: string) => {
                            return {
                                id: 'n1',
                                data: {
                                    contentType: JobContentType.CLUSTER,
                                    content: [
                                        { input: '/mock/module/a.ets', output: '/mock/output/a.abc' },
                                        { input: '/mock/module/b.ets', output: '/mock/output/b.abc' }
                                    ],
                                    arktsConfig: '',
                                    moduleName: 'test',
                                    moduleRoot: '/mock/module',
                                    declgenConfig: {},
                                    jobType: CompileJobType.DECL_ABC
                                }
                            }
                        }),
                        nodes: []
                    };
                })
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['a.ets']),
            fileToModule: new Map(),
            moduleInfos: new Map(),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };

        // delegate compile to prototype implementation which uses mocked Ets2panda
        ctx.compile = (id: string, job: any) => BaseModeClass.prototype.compile.call(ctx, id, job);
        ctx.mergeAbcFiles = jest.fn();

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(require('../../../src/util/ets2panda').Ets2panda.getInstance).toHaveBeenCalledWith(cfg);
        expect(destroySpy).toHaveBeenCalled();
        expect(ctx.mergeAbcFiles).toHaveBeenCalledWith(['/mock/out.abc']);
    });

    test('runSimultaneous succeeds and calls mergeAbcFiles', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        const pre = require('../../../src/pre_define');
        const path = require('path');

        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(() => true), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.runSimultaneous;

        const mainModule = createMockModuleInfo({ packageName: 'testPackage', entryFile: 'index.ets', arktsConfigFile: '/mock/arkts.json' });
        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => true }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['index.ets']),
            fileToModule: new Map<string, any>([['/mock/module/index.ets', mainModule]]),
            moduleInfos: new Map<string, any>([[mainModule.packageName, mainModule]]),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            cacheDir: cfg.cachePath,
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        // ensure mainPackageName is available on the context
        ctx.mainPackageName = 'testPackage';
        ctx.mergeAbcFiles = jest.fn();
        // delegate to the class's private implementation (uses mocked Ets2panda)
        ctx.compile = (id: string, job: any) => BaseModeClass.prototype.compile.call(ctx, id, job);
        ctx.mergeAbcFiles = jest.fn();

        const expectedIntermediate = path.resolve(cfg.cachePath, pre.MERGED_INTERMEDIATE_FILE);

        await expect(fn.call(ctx)).resolves.toBeUndefined();
        expect(require('../../../src/util/ets2panda').Ets2panda.getInstance).toHaveBeenCalledWith(cfg);
        expect(destroySpy).toHaveBeenCalled();
        expect(ctx.mergeAbcFiles).toHaveBeenCalledWith([expectedIntermediate]);
    });

    test('runSimultaneous throws when compile fails', async () => {
        jest.resetModules();

        const loggerInstance = {
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn(),
            printErrorAndExit: jest.fn()
        };

        jest.doMock('../../../src/logger', () => {
            const actual = jest.requireActual('../../../src/logger');
            return {
                Logger: { getInstance: jest.fn(() => loggerInstance) },
                LogDataFactory: actual.LogDataFactory
            };
        });

        const { ErrorCode, DriverError } = require('../../../src/util/error');
        const { LogDataFactory } = require('../../../src/logger');
        const mockEtsLocal = { initalize: jest.fn(), compile: jest.fn(() => { throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'fail')) }), finalize: jest.fn() };
        const destroySpy = jest.fn();
        jest.doMock('../../../src/util/ets2panda', () => ({
            Ets2panda: {
                getInstance: jest.fn(() => mockEtsLocal),
                destroyInstance: destroySpy
            }
        }));

        const BaseModeModule = require('../../../src/build/base_mode');
        const BaseModeClass = BaseModeModule.BaseMode;
        const fn = (BaseModeClass as any).prototype.runSimultaneous;

        const mainModule = createMockModuleInfo({ packageName: 'testPackage', entryFile: 'index.ets', arktsConfigFile: '/mock/arkts.json' });
        const cfg = createMockBuildConfig();
        const mockDepAnalyzerInstance = {
            getGraph: jest.fn(() => ({ hasNodes: () => true }))
        };
        const ctx: any = {
            buildConfig: cfg,
            entryFiles: new Set(['index.ets']),
            fileToModule: new Map<string, any>([['/mock/module/index.ets', mainModule]]),
            moduleInfos: new Map<string, any>([['testPackage', mainModule]]),
            abcFiles: new Set(),
            mergedAbcFile: '/tmp/merged.abc',
            logger: loggerInstance,
            statsRecorder: { record: jest.fn() },
            cacheDir: cfg.cachePath,
            moduleType: 'hap',
            getDepAnalyzer: jest.fn(() => mockDepAnalyzerInstance)
        };
        // ensure mainPackageName is available on the context
        ctx.mainPackageName = 'testPackage';
        // delegate to the class's private implementation (uses mocked Ets2panda)
        ctx.compile = (id: string, job: any) => BaseModeClass.prototype.compile.call(ctx, id, job);
        // ensure mergeAbcFiles exists to avoid undefined when the method unwinds
        ctx.mergeAbcFiles = jest.fn();

        await expect(fn.call(ctx)).rejects.toThrow('Simultaneous build failed.');
        expect(destroySpy).toHaveBeenCalled();
    });

    test('collectModuleDependencies fills dynamic and static maps', () => {
        const { LANGUAGE_VERSION } = require('../../../src/pre_define');

        const modA = createMockModuleInfo({ packageName: 'A', language: LANGUAGE_VERSION.ARKTS_HYBRID, dependencies: ['B'] });
        // make B hybrid so it appears in both static and dynamic maps for test
        const modB = createMockModuleInfo({ packageName: 'B', language: LANGUAGE_VERSION.ARKTS_HYBRID, dependencies: [] });

        (testMode as any).moduleInfos = new Map<string, any>([['A', modA], ['B', modB]]);

        (testMode as any).collectModuleDependencies();

        const a = (testMode as any).moduleInfos.get('A');
        expect(a.dynamicDependencyModules.has('B')).toBe(true);
        expect(a.staticDependencyModules.has('B')).toBe(true);
    });

    describe('resolvePackageName', () => {
        test('returns original packageName when originalPackageNameMap is undefined', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const packageName = 'com.example.test';
            const result = (mode as any).resolvePackageName(packageName, undefined);
            expect(result).toBe('com.example.test');
        });

        test('returns mapped value when packageName exists in map', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const packageName = 'alias1';
            const originalPackageNameMap = new Map<string, string>([
                ['alias1', 'com.example.real1'],
                ['alias2', 'com.example.real2']
            ]);
            const result = (mode as any).resolvePackageName(packageName, originalPackageNameMap);
            expect(result).toBe('com.example.real1');
        });

        test('returns original packageName when not found in map', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const packageName = 'unknown';
            const originalPackageNameMap = new Map<string, string>([
                ['alias1', 'com.example.real1']
            ]);
            const result = (mode as any).resolvePackageName(packageName, originalPackageNameMap);
            expect(result).toBe('unknown');
        });
    });

    describe('collectModuleInfos with alias package names', () => {
        test('resolves dependency package names using originalPackageNameMap', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const dep1 = {
                packageName: 'com.real.dep1',
                moduleName: 'depModule1',
                modulePath: '/mock/com.real.dep1',
                moduleType: 'hap',
                sourceRoots: ['srcsrc'],
                entryFile: '/mock/com.real.dep1/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep2 = {
                packageName: 'com.real.dep2',
                moduleName: 'depModule2',
                modulePath: '/mock/com.real.dep2',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/com.real.dep2/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep = {
                packageName: 'depPkg',
                moduleName: 'depModule',
                modulePath: '/mock/depPkg',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['depAlias1', 'depAlias2'],
                originalPackageNameMap: new Map<string, string>([
                    ['depAlias1', 'com.real.dep1'],
                    ['depAlias2', 'com.real.dep2']
                ])
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep, dep1, dep2]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const depModule = (mode as any).moduleInfos.get('depPkg');
            expect(depModule.dependencies).toEqual(['com.real.dep1', 'com.real.dep2']);
        });

        test('adds resolved dependency package names to main module dependencies', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const dep = {
                packageName: 'depPkg',
                moduleName: 'depModule',
                modulePath: '/mock/depPkg',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const mainModule = (mode as any).moduleInfos.get('mainPkg');
            expect(mainModule.dependencies).toContain('depPkg');
        });

        test('maintains backward compatibility when originalPackageNameMap is undefined', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const dep1 = {
                packageName: 'dep1',
                moduleName: 'depModule1',
                modulePath: '/mock/dep1',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/dep1/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep2 = {
                packageName: 'dep2',
                moduleName: 'depModule2',
                modulePath: '/mock/dep2',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/dep2/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep = {
                packageName: 'depPkg',
                moduleName: 'depModule',
                modulePath: '/mock/depPkg',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['dep1', 'dep2']
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep, dep1, dep2]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const depModule = (mode as any).moduleInfos.get('depPkg');
            expect(depModule.dependencies).toEqual(['dep1', 'dep2']);
        });

        test('preserves originalPackageNameMap in ModuleInfo', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const originalMap = new Map<string, string>([
                ['alias1', 'com.real1']
            ]);

            const dep = {
                packageName: 'depPkg',
                moduleName: 'depModule',
                modulePath: '/mock/depPkg',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: [],
                originalPackageNameMap: originalMap
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const depModule = (mode as any).moduleInfos.get('depPkg');
            expect(depModule.originalPackageNameMap).toBe(originalMap);
        });
    });

    describe('getMainModuleInfo with alias package names', () => {
        test('resolves main module dependencies using originalPackageNameMap', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const originalMap = new Map<string, string>([
                ['alias1', 'com.real.dep1'],
                ['alias2', 'com.real.dep2']
            ]);

            const mainModuleInfo = {
                packageName: 'mainPkg',
                moduleName: 'mainModule',
                modulePath: '/mock/main',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: 'index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['alias1', 'alias2'],
                originalPackageNameMap: originalMap
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [mainModuleInfo]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            const result = (mode as any).getMainModuleInfo();

            expect(result.dependencies).toEqual(['com.real.dep1', 'com.real.dep2']);
        });

        test('maintains backward compatibility when originalPackageNameMap is undefined', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const mainModuleInfo = {
                packageName: 'mainPkg',
                moduleName: 'mainModule',
                modulePath: '/mock/main',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: 'index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['dep1', 'dep2']
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [mainModuleInfo]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            const result = (mode as any).getMainModuleInfo();

            expect(result.dependencies).toEqual(['dep1', 'dep2']);
        });

        test('preserves originalPackageNameMap in main ModuleInfo', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const originalMap = new Map<string, string>([
                ['alias1', 'com.real1']
            ]);

            const mainModuleInfo = {
                packageName: 'mainPkg',
                moduleName: 'mainModule',
                modulePath: '/mock/main',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: 'index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: [],
                originalPackageNameMap: originalMap
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [mainModuleInfo]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            const result = (mode as any).getMainModuleInfo();

            expect(result.originalPackageNameMap).toBe(originalMap);
        });
    });

    describe('Integration: alias package names in module collection', () => {
        test('resolves package names across multiple dependency levels', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const dep2 = {
                packageName: 'depPkg2',
                moduleName: 'depModule2',
                modulePath: '/mock/depPkg2',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg2/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep1 = {
                packageName: 'depPkg1',
                moduleName: 'depModule1',
                modulePath: '/mock/depPkg1',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg1/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['depAlias2'],
                originalPackageNameMap: new Map<string, string>([
                    ['depAlias2', 'depPkg2']
                ])
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep1, dep2]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const dep1Module = (mode as any).moduleInfos.get('depPkg1');
            expect(dep1Module.dependencies).toEqual(['depPkg2']);
            expect(dep1Module.originalPackageNameMap).toBeDefined();

            const dep2Module = (mode as any).moduleInfos.get('depPkg2');
            expect(dep2Module.dependencies).toEqual([]);
        });

        test('handles mixed alias and real package names', () => {
            const config = createMockBuildConfig();
            const mode = new TestBaseMode(config);
            const { LANGUAGE_VERSION } = require('../../../src/pre_define');

            const dep2 = {
                packageName: 'depPkg2',
                moduleName: 'depModule2',
                modulePath: '/mock/depPkg2',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg2/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const comRealDep1 = {
                packageName: 'com.real.dep1',
                moduleName: 'comRealDep1Module',
                modulePath: '/mock/com.real.dep1',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/com.real.dep1/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: []
            };

            const dep1 = {
                packageName: 'depPkg1',
                moduleName: 'depModule1',
                modulePath: '/mock/depPkg1',
                moduleType: 'hap',
                sourceRoots: ['src'],
                entryFile: '/mock/depPkg1/index.ets',
                language: LANGUAGE_VERSION.ARKTS_1_2,
                dependencies: ['alias1', 'depPkg2'],
                originalPackageNameMap: new Map<string, string>([
                    ['alias1', 'com.real.dep1']
                ])
            };

            const newConfig = createMockBuildConfig({
                packageName: 'mainPkg',
                moduleRootPath: '/mock/main',
                sourceRoots: ['src'],
                dependencyModuleList: [dep1, dep2, comRealDep1]
            });

            (mode as any).buildConfig = newConfig;
            (mode as any).moduleInfos = new Map<string, any>();

            (mode as any).collectModuleInfos();

            const dep1Module = (mode as any).moduleInfos.get('depPkg1');
            expect(dep1Module.dependencies).toEqual(['com.real.dep1', 'depPkg2']);
        });
    });
});
