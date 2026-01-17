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

    public testGetOutputFilePaths(job: DeclgenV1JobInfo): {declEtsOutputPath: string, glueCodeOutputPath: string} {
        return this.getOutputFilePaths(job);
    }

    public async testNeedsBackup(job: DeclgenV1JobInfo): Promise<{needsDeclBackup: boolean; needsGlueCodeBackup: boolean}> {
        return this.needsBackup(job);
    }

    public async testBackupFiles(job: DeclgenV1JobInfo, needsDecl: boolean, needsGlue: boolean): Promise<void> {
        return this.backupFiles(job, needsDecl, needsGlue);
    }

    public async testUpdateDeclFileMapAsync(job: DeclgenV1JobInfo): Promise<void> {
        return this.updateDeclFileMapAsync(job);
    }

    public testNeedsRegeneration(job: DeclgenV1JobInfo): boolean {
        return (this as any).needsRegeneration(job);
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
        const job = createMockDeclgenV1JobInfo({
            fileList: ['/test/module/root/src/components/MyComponent.ets'],
            fileInfo: {
                input: '/test/module/root/src/components/MyComponent.ets',
                output: '',
                arktsConfig: '',
                moduleName: 'test-package',
                moduleRoot: '/test/module/root'
            },
            declgenConfig: {output: '/test/declgen/v1', bridgeCode: '/test/bridge/code'}
        });

        (mockChangeDeclgenFileExtension as jest.Mock)
            .mockReturnValueOnce('/test/declgen/v1/test-package/src/components/MyComponent.d.ets')
            .mockReturnValueOnce('/test/bridge/code/test-package/src/components/MyComponent.ts');

        const result = testMode.testGetOutputFilePaths(job);

        expect(result.declEtsOutputPath).toBe('/test/declgen/v1/test-package/src/components/MyComponent.d.ets');
        expect(result.glueCodeOutputPath).toBe('/test/bridge/code/test-package/src/components/MyComponent.ts');
    });

    test('needsBackup returns false when files do not exist', async () => {
        const job = createMockDeclgenV1JobInfo();

        const result = await testMode.testNeedsBackup(job);

        expect(result.needsDeclBackup).toBe(false);
        expect(result.needsGlueCodeBackup).toBe(false);
    });

    test('needsBackup returns true when declaration file timestamp changed externally', async () => {
        const job = createMockDeclgenV1JobInfo();
        const currentTime = Date.now();

        testMode.getDeclFileMap().set(job.fileList[0], {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: job.fileList[0],
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

        const result = await testMode.testNeedsBackup(job);

        expect(result.needsDeclBackup).toBe(true);
        expect(result.needsGlueCodeBackup).toBe(true);
    });

    test('needsBackup returns false when declaration file timestamp unchanged', async () => {
        const job = createMockDeclgenV1JobInfo();
        const currentTime = Date.now();

        testMode.getDeclFileMap().set(job.fileList[0], {
            delFilePath: '/test/declgen/v1/test-package/src/file1.d.ets',
            declLastModified: currentTime - 1000,
            glueCodeFilePath: '/test/bridge/code/test-package/src/file1.ts',
            glueCodeLastModified: currentTime - 1000,
            sourceFilePath: job.fileList[0],
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

        const result = await testMode.testNeedsBackup(job);

        expect(result.needsDeclBackup).toBe(false);
        expect(result.needsGlueCodeBackup).toBe(false);
    });

    test('backupFiles does backup when needed', async () => {
        const job = createMockDeclgenV1JobInfo();

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(job, true, true);

        expect(fs.existsSync).toHaveBeenCalledTimes(2);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(2);
    });

    test('backupFiles only backs up declaration file when needed', async () => {
        const job = createMockDeclgenV1JobInfo();

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(job, true, false);

        expect(fs.existsSync).toHaveBeenCalledTimes(1);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(1);
    });

    test('backupFiles only backs up glue code file when needed', async () => {
        const job = createMockDeclgenV1JobInfo();

        (fs.existsSync as jest.Mock).mockReturnValue(true);

        await testMode.testBackupFiles(job, false, true);

        expect(fs.existsSync).toHaveBeenCalledTimes(1);
        expect(fs.promises.copyFile).toHaveBeenCalledTimes(1);
    });

    test('updateDeclFileMapAsync updates declaration file map when files exist', async () => {
        const job = createMockDeclgenV1JobInfo();
        const currentTime = Date.now();

        (fs.promises.stat as jest.Mock)
            .mockResolvedValueOnce({mtimeMs: currentTime - 1000})
            .mockResolvedValueOnce({mtimeMs: currentTime - 500})
            .mockResolvedValueOnce({mtimeMs: currentTime - 200});

        await testMode.testUpdateDeclFileMapAsync(job);

        const fileInfo = testMode.getDeclFileMap().get(job.fileList[0]);

        expect(fileInfo?.declLastModified).toBe(currentTime - 500);
        expect(fileInfo?.glueCodeLastModified).toBe(currentTime - 200);
        expect(fileInfo?.sourceFileLastModified).toBe(currentTime - 1000);
    });

    test('updateDeclFileMapAsync handles missing output files gracefully', async () => {
        const job = createMockDeclgenV1JobInfo();
        const currentTime = Date.now();

        (fs.promises.stat as jest.Mock)
            .mockResolvedValueOnce({mtimeMs: currentTime - 1000})
            .mockRejectedValueOnce(new Error('File not found'))
            .mockResolvedValueOnce({mtimeMs: currentTime - 200});

        await testMode.testUpdateDeclFileMapAsync(job);

        const fileInfo = testMode.getDeclFileMap().get(job.fileList[0]);

        expect(fileInfo?.declLastModified).toBe(null);
        expect(fileInfo?.glueCodeLastModified).toBe(currentTime - 200);
        expect(fileInfo?.sourceFileLastModified).toBe(currentTime - 1000);
    });

    test('needsRegeneration returns true when source file not in map', () => {
        const job = createMockDeclgenV1JobInfo();
        const currentTime = Date.now();

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime});

        const result = testMode.testNeedsRegeneration(job);

        expect(result).toBe(true);
    });

    test('needsRegeneration returns true when source file modified', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const job = createMockDeclgenV1JobInfo({fileList: [sourceFile]});
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

        const result = testMode.testNeedsRegeneration(job);

        expect(result).toBe(true);
    });

    test('needsRegeneration returns false when source file unchanged', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const job = createMockDeclgenV1JobInfo({fileList: [sourceFile]});
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

        const result = testMode.testNeedsRegeneration(job);

        expect(result).toBe(false);
    });

    test('needsRegeneration returns true when sourceFileLastModified is null', () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const job = createMockDeclgenV1JobInfo({fileList: [sourceFile]});
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

        const result = testMode.testNeedsRegeneration(job);

        expect(result).toBe(true);
    });

    test('first generation scenario: needsRegeneration returns true, needsBackup returns false', async () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const job = createMockDeclgenV1JobInfo({fileList: [sourceFile]});
        const currentTime = Date.now();

        testMode.getDeclFileMap().clear();

        (fs.statSync as jest.Mock).mockReturnValue({mtimeMs: currentTime});

        const needsGen = testMode.testNeedsRegeneration(job);
        expect(needsGen).toBe(true);

        const backupResult = await testMode.testNeedsBackup(job);
        expect(backupResult.needsDeclBackup).toBe(false);
        expect(backupResult.needsGlueCodeBackup).toBe(false);
    });

    test('regeneration scenario with external modification', async () => {
        const sourceFile = '/test/module/root/src/file1.ets';
        const job = createMockDeclgenV1JobInfo({fileList: [sourceFile]});
        const currentTime = Date.now();

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

        const needsGen = testMode.testNeedsRegeneration(job);
        expect(needsGen).toBe(true);

        const backupResult = await testMode.testNeedsBackup(job);
        expect(backupResult.needsDeclBackup).toBe(true);
        expect(backupResult.needsGlueCodeBackup).toBe(true);

        (fs.existsSync as jest.Mock).mockReturnValue(true);
        await testMode.testBackupFiles(job, true, true);

        expect(fs.existsSync).toHaveBeenCalled();
        expect(fs.promises.copyFile).toHaveBeenCalled();
    });
});
