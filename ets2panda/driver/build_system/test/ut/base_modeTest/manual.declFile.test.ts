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
import {BaseMode} from '../../../src/build/base_mode';
import {
    BuildConfig,
    ModuleInfo,
    DeclgenV1JobInfo,
    DeclFileInfo,
    OHOS_MODULE_TYPE,
    BUILD_MODE
} from '../../../src/types';

jest.mock('fs', () => {
    const actualFs = jest.requireActual('fs');
    return {
        ...actualFs,
        existsSync: jest.fn(() => false),
        readFileSync: jest.fn(() => '{}'),
        writeFileSync: jest.fn(() => undefined),
        statSync: jest.fn(() => ({mtimeMs: Date.now()})),
        promises: {
            mkdir: jest.fn(() => Promise.resolve()),
            writeFile: jest.fn(() => Promise.resolve()),
            access: jest.fn(() => Promise.reject(new Error('File does not exist'))),
            stat: jest.fn(() => Promise.resolve({mtimeMs: Date.now()})),
            copyFile: jest.fn(() => Promise.resolve())
        }
    };
});

import * as fs from 'fs';

jest.mock('../../../src/util/utils', () => {
    const actualUtils = jest.requireActual('../../../src/util/utils');
    const mockChangeDeclgenFileExtension = jest.fn((file: string, ext: string) => {
        return file.replace(/\.[^/.]+$/, ext);
    });

    return {...actualUtils, ensurePathExists: jest.fn(), changeDeclgenFileExtension: mockChangeDeclgenFileExtension};
});

jest.mock(
    '../../../src/logger',
    () => ({
        Logger: {
            getInstance: jest.fn(
                () => ({printDebug: jest.fn(), printInfo: jest.fn(), printWarn: jest.fn(), printError: jest.fn()}))
        }
    })
);

jest.mock('../../../src/build/generate_arktsconfig',
    () => ({
        ArkTSConfigGenerator: {
            getInstance: jest.fn(() => ({
                generateArkTSConfigFile: jest.fn(),
                getArktsConfigByPackageName: jest.fn().mockReturnValue({
                    object: {},
                    mergeArktsConfig: jest.fn(),
                    mergeArktsConfigByDependencies: jest.fn()
                }),
                addPathSection: jest.fn(),
                addDependenciesSection: jest.fn(),
                addCompilerOptionsSection: jest.fn(),
                addReferencesSection: jest.fn(),
                mergeArktsConfigByDependencies: jest.fn()
            }))
        }
    })
);

jest.mock('../../../src/plugins/plugins_driver',
    () => ({PluginDriver: {getInstance: jest.fn(() => ({runPluginHook: jest.fn()}))}}));

jest.mock('../../../src/util/TaskManager', () => {
    const mockStartWorkers = jest.fn();
    const mockInitTaskQueue = jest.fn();
    const mockMarkTaskAsSkipped = jest.fn();
    const mockFinish = jest.fn().mockResolvedValue(true);
    const mockShutdownWorkers = jest.fn();

    const MockTaskManager = jest.fn().mockImplementation(() => {
        return {
            startWorkers: mockStartWorkers,
            buildGraph: undefined,
            initTaskQueue: mockInitTaskQueue,
            markTaskAsSkipped: mockMarkTaskAsSkipped,
            finish: mockFinish,
            shutdownWorkers: mockShutdownWorkers
        };
    });

    return {
        TaskManager: MockTaskManager,
        DriverProcessFactory: jest.fn().mockImplementation(() => ({
            spawnWorker: jest.fn().mockReturnValue({
                on: jest.fn().mockReturnThis(),
                send: jest.fn(),
                stop: jest.fn(),
                getId: jest.fn(),
                getWorkerPath: jest.fn(),
                spawnNewInstance: jest.fn()
            })
        }))
    };
});

jest.mock('../../../src/dependency_analyzer', () => {
    const createMockDependencyAnalyzerInstance = () => ({
        getGraph: jest.fn()
    });

    let mockInstance: any;

    const MockDependencyAnalyzer = jest.fn().mockImplementation(() => {
        mockInstance = createMockDependencyAnalyzerInstance();
        return mockInstance;
    });

    (MockDependencyAnalyzer as any).getLastInstance = () => mockInstance;

    return {
        DependencyAnalyzer: MockDependencyAnalyzer
    };
});

jest.mock('../../../src/util/graph', () => {
    class MockGraph<T> {
        nodes: Set<any>;

        constructor(nodes: any[] = []) {
            this.nodes = new Set(nodes);
        }

        filter(predicate: (node: any) => boolean): MockGraph<T> {
            const filteredNodes = Array.from(this.nodes).filter(predicate);
            return new MockGraph<T>(filteredNodes);
        }

        getNodeById(id: string): any {
            return Array.from(this.nodes).find((n: any) => n.id === id);
        }

        verify(): void {}

        hasNodes(): boolean {
            return this.nodes.size > 0;
        }
    }

    return {
        Graph: {
            createGraphFromNodes: jest.fn().mockImplementation((nodes: any[]) => {
                return new MockGraph(nodes);
            })
        }
    };
});

class TestableBaseMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }

    public overrideLoadDeclFileMap(): void {
        return this.loadDeclFileMap();
    }

    public overrideSaveDeclFileMap(): Promise<void> {
        return this.saveDeclFileMap();
    }

    public testGetOutputFilePaths(file: string): {declEtsOutputPath: string, glueCodeOutputPath: string} {
        return (this as any).getOutputFilePaths(file);
    }

    public async testNeedsBackup(file: string): Promise<{needsDeclBackup: boolean; needsGlueCodeBackup: boolean}> {
        return (this as any).needsBackup(file);
    }

    public async testBackupFiles(file: string, needsDecl: boolean, needsGlue: boolean): Promise<void> {
        return (this as any).backupFiles(file, needsDecl, needsGlue);
    }

    public async testUpdateDeclFileMapAsync(file: string): Promise<void> {
        return (this as any).updateDeclFileMapAsync(file);
    }

    public testNeedsRegeneration(file: string): boolean {
        return (this as any).needsRegeneration(file);
    }

    public getDeclFileMap() {
        return this.declFileMap;
    }

    public setFileToModule(filePath: string, moduleInfo: ModuleInfo) {
        (this as any).fileToModule.set(filePath, moduleInfo);
    }

    public setModuleInfo(packageName: string, moduleInfo: ModuleInfo) {
        (this as any).moduleInfos.set(packageName, moduleInfo);
    }

    public getModuleForFile(filePath: string): ModuleInfo|undefined {
        return (this as any).fileToModule.get(filePath);
    }
}

function createMockBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
    return {
        cachePath: '/test/cache',
        declgenV1OutPath: '/test/declgen/v1',
        declgenBridgeCodePath: '/test/bridge/code',
        declgenV2OutPath: '/test/declgen/v2',
        entryFile: 'index.ets',
        packageName: 'test-package',
        moduleRootPath: '/test/module/root',
        loaderOutPath: '/test/output',
        abcLinkerPath: '/test/linker',
        compileFiles: [],
        moduleType: OHOS_MODULE_TYPE.HAR,
        sourceRoots: ['./'],
        hasMainModule: false,
        enableDeclgenEts2Ts: false,
        buildMode: BUILD_MODE.DEBUG,
        dependencyModuleList: [],
        frameworkMode: false,
        genDeclAnnotations: true,
        skipDeclCheck: true,
        byteCodeHar: false,
        dumpDependencyGraph: false,
        dumpPerf: false,
        recordType: 'OFF',
        isBuildConfigModified: false,
        outputDir: '/test/output',
        taskQueueCapacity: 4,
        ...overrides
    } as BuildConfig;
}

function createMockDeclgenV1JobInfo(overrides: Partial<DeclgenV1JobInfo> = {}): DeclgenV1JobInfo {
    return {
        fileList: ['/test/module/root/src/file1.ets'],
        fileInfo: {
            input: '/test/module/root/src/file1.ets',
            output: '',
            arktsConfig: '',
            moduleName: 'test-package',
            moduleRoot: '/test/module/root'
        },
        declgenConfig: {output: '/test/declgen/v1', bridgeCode: '/test/bridge/code'},
        ...overrides
    } as DeclgenV1JobInfo;
}

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        isMainModule: false,
        packageName: 'test-package',
        moduleRootPath: '/test/module/root',
        moduleType: OHOS_MODULE_TYPE.HAR,
        sourceRoots: ['./'],
        entryFile: 'index.ets',
        arktsConfigFile: '/test/cache/test-package/arktsconfig.json',
        dependencies: [],
        staticDependencyModules: new Map(),
        dynamicDependencyModules: new Map(),
        language: 'ARKTS_1_2',
        declgenV1OutPath: '/test/declgen/v1',
        declgenBridgeCodePath: '/test/bridge/code',
        declgenV2OutPath: '/test/declgen/v2',
        byteCodeHar: false,
        staticFiles: [],
        ...overrides
    } as ModuleInfo;
}

describe('BaseMode declaration file map management tests', () => {
    let testMode: TestableBaseMode;
    const mockChangeDeclgenFileExtension = require('../../../src/util/utils').changeDeclgenFileExtension;

    beforeEach(() => {
        jest.clearAllMocks();
        (fs.writeFileSync as jest.Mock).mockImplementation(() => undefined);
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('{}');
        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: Date.now()});

        (fs.promises.mkdir as jest.Mock).mockResolvedValue(undefined);
        (fs.promises.writeFile as jest.Mock).mockResolvedValue(undefined);
        (fs.promises.access as jest.Mock).mockRejectedValue(new Error('File does not exist'));
        (fs.promises.stat as jest.Mock).mockResolvedValue({mtimeMs: Date.now()});
        (fs.promises.copyFile as jest.Mock).mockResolvedValue(undefined);

        const config = createMockBuildConfig();
        testMode = new TestableBaseMode(config);
        (mockChangeDeclgenFileExtension as jest.Mock).mockClear();
    });

    test('loadDeclFileMap loads empty map when file does not exist', () => {
        (fs.existsSync as jest.Mock).mockReturnValue(false);

        testMode.overrideLoadDeclFileMap();

        expect(testMode.getDeclFileMap().size).toBe(0);
    });

    test('loadDeclFileMap loads declaration file map correctly', () => {
        const mockData = {
            '/test/source/file1.ets': {
                delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
                declLastModified: 1700000000000,
                glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
                glueCodeLastModified: 1700000001000,
                sourceFilePath: '/test/source/file1.ets',
                sourceFileLastModified: 1700000000500
            }
        };

        (fs.existsSync as jest.Mock).mockReturnValue(true);
        (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify(mockData));

        testMode.overrideLoadDeclFileMap();

        expect(testMode.getDeclFileMap().size).toBe(1);

        const fileInfo = testMode.getDeclFileMap().get('/test/source/file1.ets');
        expect(fileInfo?.sourceFileLastModified).toBe(1700000000500);
    });

    test('saveDeclFileMap saves empty declaration file map', async () => {
        await testMode.overrideSaveDeclFileMap();

        expect(fs.promises.mkdir).toHaveBeenCalledWith('/test/cache', {recursive: true});
        expect(fs.promises.writeFile).toHaveBeenCalledWith('/test/cache/decl_file_map.json', '{}');
    });

    test('getOutputFilePaths generates correct paths for ETS file', () => {
        const file = '/test/module/root/src/components/MyComponent.ets';
        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        const result = testMode.testGetOutputFilePaths(file);

        expect(result.declEtsOutputPath).toBe('/test/declgen/v1/test-package/src/components/MyComponent.d.ets');
        expect(result.glueCodeOutputPath).toBe('/test/bridge/code/test-package/src/components/MyComponent.ts');
    });

    test('needsBackup returns false when files do not exist', async () => {
        const file = '/test/module/root/src/file1.ets';
        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        const result = await testMode.testNeedsBackup(file);

        expect(result.needsDeclBackup).toBe(false);
        expect(result.needsGlueCodeBackup).toBe(false);
    });

    test('needsBackup returns true when declaration file timestamp changed externally', async () => {
        const file = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();
        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        testMode.getDeclFileMap().set(file, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: file,
            sourceFileLastModified: currentTime - 2000
        });

        const mockStat = {
            isFile: () => true,
            mtimeMs: currentTime - 500,
            isDirectory: () => false,
            isSymbolicLink: () => false,
            size: 1024,
        };

        (fs.promises.stat as jest.Mock).mockResolvedValue(mockStat);

        const result = await testMode.testNeedsBackup(file);

        expect(result.needsDeclBackup).toBe(true);
        expect(result.needsGlueCodeBackup).toBe(true);
    });

    test('needsBackup returns false when declaration file timestamp unchanged', async () => {
        const file = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        testMode.getDeclFileMap().set(file, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: file,
            sourceFileLastModified: currentTime - 2000
        });

        const mockStat = {
            isFile: () => true,
            mtimeMs: currentTime - 1000,
            isDirectory: () => false,
            isSymbolicLink: () => false,
            size: 1024,
        };

        (fs.promises.stat as jest.Mock).mockResolvedValue(mockStat);

        const result = await testMode.testNeedsBackup(file);

        expect(result.needsDeclBackup).toBe(false);
        expect(result.needsGlueCodeBackup).toBe(false);
    });

    test('backupFilesForFile does backup when needed', async () => {
        const file = '/test/module/root/src/file1.ets';
        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(file, true, true);

        expect(fs.existsSync).toHaveBeenCalledTimes(2);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(2);
    });

    test('backupFiles only backs up declaration file when needed', async () => {
        const file = '/test/module/root/src/file1.ets';

        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(file, true, false);

        expect(fs.existsSync).toHaveBeenCalledTimes(1);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(1);
    });

    test('backupFiles only backs up glue code file when needed', async () => {
        const file = '/test/module/root/src/file1.ets';

        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(file, false, true);

        expect(fs.existsSync).toHaveBeenCalledTimes(1);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(1);
    });

    test('updateDeclFileMapAsync updates declaration file map when files exist', async () => {
        const file = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();
        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        (fs.promises.stat as jest.Mock)
            .mockResolvedValueOnce({mtimeMs: currentTime - 1000})
            .mockResolvedValueOnce({mtimeMs: currentTime - 500})
            .mockResolvedValueOnce({mtimeMs: currentTime - 200});

        await testMode.testUpdateDeclFileMapAsync(file);

        const fileInfo = testMode.getDeclFileMap().get(file);

        expect(fileInfo?.declLastModified).toBe(currentTime - 500);
        expect(fileInfo?.glueCodeLastModified).toBe(currentTime - 200);
        expect(fileInfo?.sourceFileLastModified).toBe(currentTime - 1000);
    });

    test('updateDeclFileMapAsync handles missing output files gracefully', async () => {
        const file = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.setFileToModule(file, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        (fs.promises.stat as jest.Mock)
            .mockResolvedValueOnce({mtimeMs: currentTime - 1000})
            .mockRejectedValueOnce(new Error('File not found'))
            .mockResolvedValueOnce({mtimeMs: currentTime - 200});

        await testMode.testUpdateDeclFileMapAsync(file);

        const fileInfo = testMode.getDeclFileMap().get(file);

        expect(fileInfo?.declLastModified).toBe(null);
        expect(fileInfo?.glueCodeLastModified).toBe(currentTime - 200);
        expect(fileInfo?.sourceFileLastModified).toBe(currentTime - 1000);
    });

    test('needsRegeneration returns true when source file not in map', () => {
        const file = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime});

        const result = testMode.testNeedsRegeneration(file);

        expect(result).toBe(true);
    });

    test('needsRegeneration returns true when source file modified', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.getDeclFileMap().set(sourceFile, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: sourceFile,
            sourceFileLastModified: currentTime - 2000
        });

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime - 500});

        const result = testMode.testNeedsRegeneration(sourceFile);

        expect(result).toBe(true);
    });

    test('needsRegeneration returns false when source file unchanged', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.getDeclFileMap().set(sourceFile, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: sourceFile,
            sourceFileLastModified: currentTime - 1000
        });

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime - 1000});

        const result = testMode.testNeedsRegeneration(sourceFile);

        expect(result).toBe(false);
    });

    test('needsRegeneration returns true when sourceFileLastModified is null', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.getDeclFileMap().set(sourceFile, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: sourceFile,
            sourceFileLastModified: null
        });

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime});

        const result = testMode.testNeedsRegeneration(sourceFile);

        expect(result).toBe(true);
    });

    test('first generation scenario: needsRegeneration returns true, needsBackupForFile returns false',
         async () => {
             const sourceFile = '/test/module/root/src/file1.ets';
             const currentTime = Date.now();
             testMode.setFileToModule(sourceFile, createMockModuleInfo({
                                          packageName: 'test-package',
                                          moduleRootPath: '/test/module/root',
                                          declgenV1OutPath: '/test/declgen/v1',
                                          declgenBridgeCodePath: '/test/bridge/code'
                                      }));

             testMode.getDeclFileMap().clear();

             (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime});

             const needsGen = testMode.testNeedsRegeneration(sourceFile);
             expect(needsGen).toBe(true);

             const backupResult = await testMode.testNeedsBackup(sourceFile);
             expect(backupResult.needsDeclBackup).toBe(false);
             expect(backupResult.needsGlueCodeBackup).toBe(false);
         });

    test('regeneration scenario with external modification', async () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const currentTime = Date.now();

        testMode.setFileToModule(sourceFile, createMockModuleInfo({
                                     packageName: 'test-package',
                                     moduleRootPath: '/test/module/root',
                                     declgenV1OutPath: '/test/declgen/v1',
                                     declgenBridgeCodePath: '/test/bridge/code'
                                 }));

        testMode.getDeclFileMap().set(sourceFile, {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 2000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 2000,
            sourceFilePath: sourceFile,
            sourceFileLastModified: currentTime - 1500
        });

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime - 500});

        const mockStat = {
            isFile: () => true,
            mtimeMs: currentTime - 600,
            isDirectory: () => false,
            isSymbolicLink: () => false,
            size: 1024,
        };

        (fs.promises.stat as jest.Mock).mockResolvedValue(mockStat);

        const needsGen = testMode.testNeedsRegeneration(sourceFile);
        expect(needsGen).toBe(true);

        const backupResult = await testMode.testNeedsBackup(sourceFile);
        expect(backupResult.needsDeclBackup).toBe(true);
        expect(backupResult.needsGlueCodeBackup).toBe(true);

        (fs.existsSync as jest.Mock).mockReturnValue(true);
        await testMode.testBackupFiles(sourceFile, true, true);

        expect(fs.existsSync).toHaveBeenCalled();
        expect(fs.promises.copyFile).toHaveBeenCalled();
    });
    test('should handle partial regeneration in dependency chain without graph corruption', async () => {
        const basePath = '/test/module/root/src';
        const moduleRoot = '/test/module/root';
        const moduleName = 'test-package';

        const fileA = `${basePath}/A.ets`;
        const fileB = `${basePath}/B.ets`;
        const fileC = `${basePath}/C.ets`;

        const mockFileToModule = new Map();
        const mockModuleInfo = createMockModuleInfo({
            packageName: moduleName,
            moduleRootPath: moduleRoot,
            declgenV1OutPath: '/output',
            declgenBridgeCodePath: '/bridge'
        });

        mockFileToModule.set(fileA, mockModuleInfo);
        mockFileToModule.set(fileB, mockModuleInfo);
        mockFileToModule.set(fileC, mockModuleInfo);

        (testMode as any).fileToModule = mockFileToModule;
        (testMode as any).entryFiles = [fileA, fileB, fileC];
        (testMode as any).moduleInfos = new Map();

        const {TaskManager} = require('../../../src/util/TaskManager');
        const {DependencyAnalyzer} = require('../../../src/dependency_analyzer');

        (TaskManager as jest.Mock).mockClear();

        let taskManagerInstance: any;
        let capturedBuildGraph: any;
        (TaskManager as jest.Mock).mockImplementation(function() {
            const instance = {
                startWorkers: jest.fn(),
                _buildGraph: undefined as any,
                initTaskQueue: jest.fn(),
                finish: jest.fn().mockResolvedValue(true),
                shutdownWorkers: jest.fn()
            };

            Object.defineProperty(instance, 'buildGraph', {
                get: function() {
                    return this._buildGraph;
                },
                set: function(value: any) {
                    capturedBuildGraph = value;
                    this._buildGraph = value;
                },
                enumerable: true,
                configurable: true
            });

            taskManagerInstance = instance;
            return taskManagerInstance;
        });

        const mockDepAnalyzerInstance = {
            getGraph: jest.fn().mockReturnValue({
                nodes: [
                    {
                        id: 'nodeA',
                        data: {
                            fileList: [fileA],
                            fileInfo: {
                                input: fileA,
                                output: '',
                                arktsConfig: '',
                                moduleName: moduleName,
                                moduleRoot: moduleRoot
                            }
                        },
                        predecessors: new Set<string>(),
                        descendants: new Set<string>(['nodeB'])
                    },
                    {
                        id: 'nodeB',
                        data: {
                            fileList: [fileB],
                            fileInfo: {
                                input: fileB,
                                output: '',
                                arktsConfig: '',
                                moduleName: moduleName,
                                moduleRoot: moduleRoot
                            }
                        },
                        predecessors: new Set<string>(['nodeA']),
                        descendants: new Set<string>(['nodeC'])
                    },
                    {
                        id: 'nodeC',
                        data: {
                            fileList: [fileC],
                            fileInfo: {
                                input: fileC,
                                output: '',
                                arktsConfig: '',
                                moduleName: moduleName,
                                moduleRoot: moduleRoot
                            }
                        },
                        predecessors: new Set<string>(['nodeB']),
                        descendants: new Set<string>()
                    }
                ],
                hasNodes: () => true
            })
        };

        (DependencyAnalyzer as jest.Mock).mockImplementation(() => mockDepAnalyzerInstance);

        const nodeNeedsRegenerationCalls: Array<{nodeId: string, files: string[]}> = [];
        (testMode as any).nodeNeedsRegeneration = jest.fn().mockImplementation((node: any) => {
            nodeNeedsRegenerationCalls.push({nodeId: node.id, files: node.data.fileList});
            return node.data.fileList.includes(fileB);
        });

        const backupFilesCalls: string[] = [];
        (testMode as any).needsBackup = jest.fn().mockImplementation((file: string) => {
            backupFilesCalls.push(file);
            const needsDeclBackup = file === fileB;
            return Promise.resolve({needsDeclBackup, needsGlueCodeBackup: false});
        });

        const updateCalls: string[] = [];
        (testMode as any).updateDeclFileMapAsync = jest.fn().mockImplementation((file: string) => {
            updateCalls.push(file);
            return Promise.resolve();
        });

        (testMode as any).backupFiles = jest.fn();
        (testMode as any).loadDeclFileMap = jest.fn();
        (testMode as any).saveDeclFileMap = jest.fn();

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: 1000});
        (fs.existsSync as jest.Mock).mockReturnValue(true);

        testMode.declFileMap = new Map<string, any>([
            [
                fileA, {
                    delFilePath: '/output/test-package/src/A.d.ets',
                    declLastModified: 1000,
                    glueCodeFilePath: '/bridge/test-package/src/A.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileA,
                    sourceFileLastModified: 1000
                }
            ],
            [
                fileB, {
                    delFilePath: '/output/test-package/src/B.d.ets',
                    declLastModified: 1000,
                    glueCodeFilePath: '/bridge/test-package/src/B.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileB,
                    sourceFileLastModified: 1000
                }
            ],
            [
                fileC, {
                    delFilePath: '/output/test-package/src/C.d.ets',
                    declLastModified: 1000,
                    glueCodeFilePath: '/bridge/test-package/src/C.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileC,
                    sourceFileLastModified: 1000
                }
            ]
        ]);

        await (testMode as any).generateDeclarationV1Parallel();

        expect(TaskManager).toHaveBeenCalled();
        expect(taskManagerInstance.startWorkers).toHaveBeenCalled();
        expect(taskManagerInstance.initTaskQueue).toHaveBeenCalled();
        expect(taskManagerInstance.finish).toHaveBeenCalled();

        expect(capturedBuildGraph).toBeDefined();

        expect(nodeNeedsRegenerationCalls.length).toBe(3);

        expect(backupFilesCalls).toEqual([fileB]);
        expect(backupFilesCalls.length).toBe(1);

        expect((testMode as any).backupFiles).toHaveBeenCalledTimes(1);
        expect((testMode as any).backupFiles).toHaveBeenCalledWith(fileB, true, false);

        expect(updateCalls).toEqual([fileB]);
        expect(updateCalls.length).toBe(1);

        expect((testMode as any).loadDeclFileMap).toHaveBeenCalled();
        expect((testMode as any).saveDeclFileMap).toHaveBeenCalled();
    });

    test('should handle partial regeneration in circular dependency C->B->A->C', async () => {
        const basePath = '/test/module/root/src';
        const moduleRoot = '/test/module/root';
        const moduleName = 'test-package';

        const fileA = `${basePath}/A.ets`;
        const fileB = `${basePath}/B.ets`;
        const fileC = `${basePath}/C.ets`;

        const mockFileToModule = new Map();
        const mockModuleInfo = createMockModuleInfo({
            packageName: moduleName,
            moduleRootPath: moduleRoot,
            declgenV1OutPath: '/output',
            declgenBridgeCodePath: '/bridge'
        });

        mockFileToModule.set(fileA, mockModuleInfo);
        mockFileToModule.set(fileB, mockModuleInfo);
        mockFileToModule.set(fileC, mockModuleInfo);

        (testMode as any).fileToModule = mockFileToModule;
        (testMode as any).entryFiles = [fileA, fileB, fileC];
        (testMode as any).moduleInfos = new Map();

        const {TaskManager} = require('../../../src/util/TaskManager');
        const {DependencyAnalyzer} = require('../../../src/dependency_analyzer');

        (TaskManager as jest.Mock).mockClear();

        let taskManagerInstance: any;
        (TaskManager as jest.Mock).mockImplementation(function() {
            taskManagerInstance = {
                startWorkers: jest.fn(),
                buildGraph: undefined,
                initTaskQueue: jest.fn(),
                markTasksAsSkipped: jest.fn(),
                finish: jest.fn().mockResolvedValue(true),
                shutdownWorkers: jest.fn()
            };
            return taskManagerInstance;
        });

        const mockDepAnalyzerInstance = {
            getGraph: jest.fn().mockReturnValue({
                nodes: [{
                    id: 'cluster_ABC',
                    data: {
                        fileList: [fileC, fileB, fileA],
                        fileInfo:
                            {input: fileC, output: '', arktsConfig: '', moduleName: moduleName, moduleRoot: moduleRoot}
                    },
                    predecessors: new Set<string>(),
                    descendants: new Set<string>()
                }],
                hasNodes: () => true
            })
        };

        (DependencyAnalyzer as jest.Mock).mockImplementation(() => mockDepAnalyzerInstance);

        const nodeNeedsRegenerationCalls: Array<{nodeId: string, files: string[]}> = [];
        (testMode as any).nodeNeedsRegeneration = jest.fn().mockImplementation((node: any) => {
            nodeNeedsRegenerationCalls.push({nodeId: node.id, files: node.data.fileList});
            return node.data.fileList.includes(fileB);
        });

        const needsBackupCalls: string[] = [];
        (testMode as any).needsBackup = jest.fn().mockImplementation((file: string) => {
            needsBackupCalls.push(file);
            let needsDeclBackup = false;
            let needsGlueCodeBackup = false;

            if (file === fileA) {
                needsDeclBackup = true;
            } else if (file === fileB) {
                needsDeclBackup = true;
            } else if (file === fileC) {
                needsGlueCodeBackup = true;
            }

            return Promise.resolve({needsDeclBackup, needsGlueCodeBackup});
        });

        const backupFilesCalls: string[] = [];
        (testMode as any).backupFiles =
            jest.fn().mockImplementation((file: string, needsDecl: boolean, needsGlue: boolean) => {
                backupFilesCalls.push(file);
                return Promise.resolve();
            });

        const updateCalls: string[] = [];
        (testMode as any).updateDeclFileMapAsync = jest.fn().mockImplementation((file: string) => {
            updateCalls.push(file);
            return Promise.resolve();
        });

        (testMode as any).loadDeclFileMap = jest.fn();
        (testMode as any).saveDeclFileMap = jest.fn();

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: 1000});
        (fs.existsSync as jest.Mock).mockReturnValue(true);

        testMode.declFileMap = new Map<string, any>([
            [
                fileA, {
                    delFilePath: '/output/test-package/src/A.d.ets',
                    declLastModified: 1000,
                    glueCodeFilePath: '/bridge/test-package/src/A.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileA,
                    sourceFileLastModified: 1000
                }
            ],
            [
                fileB, {
                    delFilePath: '/output/test-package/src/B.d.ets',
                    declLastModified: 900,
                    glueCodeFilePath: '/bridge/test-package/src/B.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileB,
                    sourceFileLastModified: 1000
                }
            ],
            [
                fileC, {
                    delFilePath: '/output/test-package/src/C.d.ets',
                    declLastModified: 1000,
                    glueCodeFilePath: '/bridge/test-package/src/C.ts',
                    glueCodeLastModified: 1000,
                    sourceFilePath: fileC,
                    sourceFileLastModified: 1000
                }
            ]
        ]);

        await (testMode as any).generateDeclarationV1Parallel();

        expect(TaskManager).toHaveBeenCalled();
        expect(taskManagerInstance.startWorkers).toHaveBeenCalled();
        expect(taskManagerInstance.initTaskQueue).toHaveBeenCalled();
        expect(taskManagerInstance.finish).toHaveBeenCalled();

        expect(taskManagerInstance.buildGraph).toBeDefined();

        expect(nodeNeedsRegenerationCalls.length).toBe(1);
        expect(nodeNeedsRegenerationCalls[0].nodeId).toBe('cluster_ABC');

        expect(needsBackupCalls).toEqual([fileC, fileB, fileA]);
        expect(needsBackupCalls.length).toBe(3);

        expect(backupFilesCalls).toEqual([fileC, fileB, fileA]);
        expect(backupFilesCalls.length).toBe(3);

        expect((testMode as any).backupFiles).toHaveBeenCalledTimes(3);
        expect((testMode as any).backupFiles).toHaveBeenCalledWith(fileC, false, true);
        expect((testMode as any).backupFiles).toHaveBeenCalledWith(fileB, true, false);
        expect((testMode as any).backupFiles).toHaveBeenCalledWith(fileA, true, false);

        expect(updateCalls).toEqual([fileC, fileB, fileA]);
        expect(updateCalls.length).toBe(3);

        expect((testMode as any).loadDeclFileMap).toHaveBeenCalled();
        expect((testMode as any).saveDeclFileMap).toHaveBeenCalled();
    });
});

