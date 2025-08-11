/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import fs from 'fs';
import path from 'path';

import { BaseMode } from '../../../src/build/base_mode';
import {
    BuildConfig,
    BUILD_TYPE,
    BUILD_MODE,
    OHOS_MODULE_TYPE,
    ModuleInfo,
    JobInfo
} from '../../../src/types';
import {
    DependencyFileMap
} from '../../../src/dependency_analyzer'
import { BuildMode } from '../../../src/build/build_mode';
import { ErrorCode } from '../../../src/util/error';
import { Logger } from '../../../src/logger'
import * as mock from '../mock/data'
import { LANGUAGE_VERSION } from '../../../src/pre_define'

interface LogDataFactory {
    newInstance: jest.Mock;
}

function getMockMainModuleInfo(): ModuleInfo {
    return {
        isMainModule: true,
        packageName: "test",
        moduleRootPath: "/test/path",
        moduleType: OHOS_MODULE_TYPE.HAR,
        sourceRoots: ["./"],
        entryFile: "index.ets",

        arktsConfigFile: "/dist/cache/test/arktsconfig.json",
        dependencies: [],
        staticDependencyModules: new Map(),
        dynamicDependencyModules: new Map(),
        language: LANGUAGE_VERSION.ARKTS_1_2,
    }
}

interface ThreadWorker {
    postMessage: (message: any) => void;
}

beforeEach(() => {
    jest.clearAllMocks();
    process.exit = jest.fn() as any;
});

beforeAll(() => {
    const { execSync } = require('child_process');
    execSync('rimraf test/ut/mock/dist', { stdio: 'pipe' });
    const dir = path.resolve('dist/cache');
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }

    Logger.getInstance(mock.getMockLoggerGetter())

    const PluginDriver = require('../../../src/plugins/plugins_driver').PluginDriver;
    PluginDriver.getInstance = jest.fn().mockReturnValue({
        runPluginHook: jest.fn()
    });

});

// Test the functions of the base_mode.ts file.
describe('test base_mode.ts file api', () => {
    test('test collectModuleInfos', () => {
        test_collectModuleInfos();
    });

    test('test collectDependentCompileFiles002', () => {
        test_collectDependentCompileFiles002();
    });

    test('test processEntryFiles when test file not in module path branch', () => {
        test_processEntryFiles_file_not_in_module();
    });

    test('test createExternalProgramJob method branches', () => {
        test_createExternalProgramJob_branches();
    });

    test('test findStronglyConnectedComponents method branches', () => {
        test_findStronglyConnectedComponents_branches();
    });

    test('test run method', () => {
        test_runMethod();
    });

    test('test getDependentModules with missing module', () => {
        test_getDependentModules_missing_module();
    });

    test('test collectDependencyModules language branches', () => {
        test_collectDependencyModules_language_branches();
    });

    test('test initCompileQueues method', () => {
        test_initCompileQueues();
    });

    test('test addJobToQueues method', () => {
        test_addJobToQueues();
    });

    test('test dealWithDependants method', () => {
        test_dealWithDependants();
    });

    test('test getJobDependants method', () => {
        test_getJobDependants();
    });

    test('test getJobDependencies method', () => {
        test_getJobDependencies();
    });

    test('test getSerializableConfig handles bigint values', () => {
        test_getSerializableConfig();
    });

    test('test collectDependentCompileFiles', () => {
        test_collectDependentCompileFiles();
    });

    test('test isFileChanged method branches', () => {
        test_isFileChanged();
    });

    test('test loadHashCache method branches', () => {
        test_loadHashCache();
    });

    test('test updateDependantJobs method', () => {
        test_updateDependantJobs();
    });

    test('test collectModuleInfos branches001', () => {
        test_collectModuleInfos001();
    });

    test('test collectCompileFiles enableDeclgenEts2Ts false branch', () => {
        test_collectCompileFiles_enableDeclgenEts2Ts_false();
    });

    test('test collectDependentCompileFiles isFileChanged branch', () => {
        test_collectDependentCompileFiles_isFileChanged_branch();
    });

    test('collectCompileJobs should skip entry files not in compileFiles', () => {
        test_collectCompileJobs_should_skip_entry_files_not_in_compileFiles();
    });

    test('needsBackup should backUp when declgenFile modified', () => {
        test_needsBackup_declgenfile_should_backUp_when_dclgenFile_modified();
    });

    function test_needsBackup_declgenfile_should_backUp_when_dclgenFile_modified() {
        const fs = require('fs');
        jest.spyOn(fs, 'existsSync').mockImplementation(function (path) {
            return path === '/test/path/file1.d.ts' || path === '/test/path/file1.ts';
        });
        jest.spyOn(fs, 'statSync').mockImplementation(function (path) {
            if (path === '/test/path/file1.d.ts') {
                return { mtimeMs: 456 }
            } else {
                return { mtimeMs: 789 }
            }
        });

        const mockConfig = {
            packageName: 'test',
            moduleRootPath: '/test/path',
            sourceRoots: ['./'],
            loaderOutPath: './dist',
            cachePath: './dist/cache',
            dependentModuleList: [],
            buildMode: BUILD_MODE.DEBUG,
        };
        const fileInfo = {
            filePath: '/test/path/file1.ets',
            dependentFiles: [],
            abcFilePath: '/test/path/file1.abc',
            arktsConfigFile: '/test/path/arktsconfig.json',
            packageName: 'test',
        };
        const declFileInfo: DeclFileInfo = {
            delFilePath: '/test/path/declgen/file1.d.ts',
            declLastModified: 123,
            glueCodeFilePath: '/test/path/declgen/file1.ts',
            glueCodeLastModified: 123,
            sourceFilePath: '/test/path/file1.ets',
        };

        class TestBaseMode extends BaseMode {
            public run(): Promise<void> {
                return Promise.resolve();
            }
        }
        const baseMode = new TestBaseMode(mockConfig as any);

        jest.spyOn(baseMode as any, 'getOutputFilePaths').mockImplementation(() => ({
            declEtsOutputPath: '/test/path/file1.d.ts',
            glueCodeOutputPath: '/test/path/file1.ts',
        }));
        let { needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo);

        expect(fs.existsSync).toHaveBeenCalledTimes(3);
        expect(fs.statSync).toHaveBeenCalledTimes(0);
        expect(needsDeclBackup).toBe(false);
        expect(needsGlueCodeBackup).toBe(false);

        baseMode.declFileMap.set(fileInfo.filePath, declFileInfo);

        ({ needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo));

        expect(fs.existsSync).toHaveBeenCalledTimes(5);
        expect(fs.statSync).toHaveBeenCalledTimes(2);
        expect(needsDeclBackup).toBe(true);
        expect(needsGlueCodeBackup).toBe(true);

        jest.spyOn(fs, 'statSync').mockImplementation(function (path) {
            if (path === '/test/path/file1.d.ts') {
                return { mtimeMs: 123 }
            } else {
                return { mtimeMs: 789 }
            }
        });

        ({ needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo));

        expect(fs.existsSync).toHaveBeenCalledTimes(7);
        expect(fs.statSync).toHaveBeenCalledTimes(4);
        expect(needsDeclBackup).toBe(false);
        expect(needsGlueCodeBackup).toBe(true);

        jest.spyOn(fs, 'statSync').mockImplementation(function (path) {
            if (path === '/test/path/file1.d.ts') {
                return { mtimeMs: 456 }
            } else {
                return { mtimeMs: 123 }
            }
        });

        ({ needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo));

        expect(fs.existsSync).toHaveBeenCalledTimes(9);
        expect(fs.statSync).toHaveBeenCalledTimes(6);
        expect(needsDeclBackup).toBe(true);
        expect(needsGlueCodeBackup).toBe(false);

        jest.spyOn(fs, 'statSync').mockImplementation(function (path) {
            if (path === '/test/path/file1.d.ts') {
                return { mtimeMs: 123 }
            } else {
                return { mtimeMs: 123 }
            }
        });

        ({ needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo));

        expect(fs.existsSync).toHaveBeenCalledTimes(11);
        expect(fs.statSync).toHaveBeenCalledTimes(8);
        expect(needsDeclBackup).toBe(false);
        expect(needsGlueCodeBackup).toBe(false);

        baseMode.declFileMap.clear();

        ({ needsDeclBackup, needsGlueCodeBackup } = baseMode.needsBackup(fileInfo));

        expect(fs.existsSync).toHaveBeenCalledTimes(11);
        expect(fs.statSync).toHaveBeenCalledTimes(8);
        expect(needsDeclBackup).toBe(false);
        expect(needsGlueCodeBackup).toBe(false);
    }


    // NOTE: to be defined later
    // test('test checkAllTasksDone method', () => {
    //     test_checkAllTasksDone();
    // });
    //
    // test('test assignTaskToIdleWorker abcQueue branch without job', () => {
    //     test_assignTaskToIdleWorker_abcQueue_no_job();
    // });
    //
    // test('test assignTaskToIdleWorker with empty queues', () => {
    //     test_assignTaskToIdleWorker_empty_queues();
    // });
    //
    // test('test generateDeclaration method', () => {
    //     return test_generateDeclaration();
    // });
    //
    // test('test shouldSkipFile', () => {
    //     test_shouldSkipFile();
    // });
    //
    // test('test collectCompileFiles when test declaration files skip branch', () => {
    //     test_processEntryFiles_decl_ets_skip();
    // });
    //
    // test('test collectCompileFiles when test bytecode HAR branch', () => {
    //     test_collectCompileFiles_bytecode_har();
    // });
    //
    // test('test declgen method', () => {
    //     test_declgen_method();
    // });
    //
    // test('test runConcurrent method', () => {
    //     test_runConcurrent();
    // });
    //
    // test('test processAfterCompile method', () => {
    //     test_processAfterCompile();
    // });
    //
    // test('test collectAbcFileFromByteCodeHar_missing_abc_path', () => {
    //     test_collectAbcFileFromByteCodeHar_missing_abc_path();
    // });
    //
    // NOTE: to be moved to Dependency Analyzer ut
    // test('test collectCompileJobs method', () => {
    //     test_collectCompileJobs();
    // });

});

function test_collectCompileJobs_should_skip_entry_files_not_in_compileFiles() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }
    }

    const baseMode = new TestBaseMode(mockConfig as any);

    const entryFile = '/path/to/entry.ets';
    const includedFile = '/path/to/included.ets';

    (baseMode as any).entryFiles = new Set([entryFile]);
    (baseMode as any).compileFiles = new Set();

    const dependencyFileMap: {
        dependencies: { [key: string]: string[] },
        dependants: { [key: string]: string[] }
    } = {
        dependencies: {
            [entryFile]: ['dependency1.ets']
        },
        dependants: {
            [entryFile]: ['dependant1.ets']
        }
    };
    (baseMode as any).dependencyFileMap = dependencyFileMap;

    const cycleGroups = new Map();
    jest.spyOn(baseMode as any, 'findStronglyConnectedComponents').mockReturnValue(cycleGroups);

    jest.spyOn(baseMode as any, 'getJobDependencies').mockReturnValue(new Set());
    jest.spyOn(baseMode as any, 'getJobDependants').mockReturnValue(new Set());
    jest.spyOn(baseMode as any, 'dealWithDependants').mockImplementation(() => { });
    jest.spyOn(baseMode as any, 'createExternalProgramJob').mockImplementation(() => { });
    jest.spyOn(baseMode as any, 'getAbcJobId').mockImplementation((file) => `abc_${file}`);
    jest.spyOn(baseMode as any, 'getExternalProgramJobId').mockImplementation((file) => `external_${file}`);

    const jobs = {};

    (baseMode as any).collectCompileJobs(jobs);

    expect(Object.keys(jobs).length).toBe(0);
    expect((baseMode as any).getJobDependencies).not.toHaveBeenCalledWith(['dependency1.ets'], expect.anything());
    expect((baseMode as any).getJobDependants).not.toHaveBeenCalledWith(['dependant1.ets'], expect.anything());

    (baseMode as any).entryFiles.add(includedFile);
    (baseMode as any).compileFiles.add(includedFile);

    dependencyFileMap.dependencies = {
        ...dependencyFileMap.dependencies,
        [includedFile]: []
    };

    dependencyFileMap.dependants = {
        ...dependencyFileMap.dependants,
        [includedFile]: []
    };

    jest.clearAllMocks();
}

function test_collectDependentCompileFiles_isFileChanged_branch() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockConfig = {
        packageName: "test",
        moduleType: "har",
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependencyModuleList: []
    };

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testCollectDependentCompileFiles(): void {
            (this as any).collectDependentCompileFiles();
        }

        public setIsFileChanged(fn: (file: string, abcFile: string) => boolean): void {
            (this as any).isFileChanged = fn;
        }
    }

    const fs = require('fs');
    jest.spyOn(fs, 'statSync').mockReturnValue({ mtimeMs: Date.now() });
    jest.spyOn(fs, 'readFileSync').mockReturnValue('mocked file content');

    const utils = require('../../../src/util/utils');
    jest.spyOn(utils, 'getFileHash').mockReturnValue("test-hash-123");

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);

    const baseMode = new TestBaseMode(mockConfig as any);

    const testFile1 = "/test/path/file1.ets";
    const testFile2 = "/test/path/file2.ets";

    (baseMode as any).entryFiles = new Set([testFile1, testFile2]);
    (baseMode as any).cacheDir = "./dist/cache";
    (baseMode as any).hashCache = {};
    (baseMode as any).abcFiles = new Set();
    (baseMode as any).compileFiles = new Map();
    (baseMode as any).allFiles = new Map();

    (baseMode as any).moduleInfos = new Map();
    (baseMode as any).moduleInfos.set("test", {
        packageName: "test",
        moduleType: "har",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "./dist/cache/test/config.json",
        compileFileInfos: []
    });

    (baseMode as any).dependencyFileMap = {
        dependencies: {
            [testFile1]: [],
            [testFile2]: [testFile1]
        },
        dependants: {
            [testFile1]: [testFile2],
            [testFile2]: []
        }
    };

    baseMode.setIsFileChanged(() => true);
    (baseMode as any).isBuildConfigModified = false;

    baseMode.testCollectDependentCompileFiles();

    expect((baseMode as any).compileFiles.size).toBe(2);
    expect((baseMode as any).compileFiles.has(testFile1)).toBe(true);
    expect((baseMode as any).compileFiles.has(testFile2)).toBe(true);

    (baseMode as any).compileFiles.clear();
    (baseMode as any).abcFiles.clear();
    jest.restoreAllMocks();
}

// NOTE: to be defined later
/*
function test_collectAbcFileFromByteCodeHar_missing_abc_path() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn()
    };

    const LogDataFactory = {
        newInstance: jest.fn().mockReturnValue({
            code: "11410101",
            description: "abc file not found in bytecode har test-module."
        })
    };

    const ErrorCode = {
        BUILDSYSTEM_ABC_FILE_MISSING_IN_BCHAR: '11410101'
    };

    const mockConfig = {
        packageName: "main-package",
        moduleType: OHOS_MODULE_TYPE.SHARED,
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependencyModuleList: []
    };

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testCollectAbcFileFromByteCodeHar(): void {
            this.collectAbcFileFromByteCodeHar();
        }
    }

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);

    (global as any).LogDataFactory = LogDataFactory;
    (global as any).ErrorCode = ErrorCode;

    const baseMode = new TestBaseMode(mockConfig as any);
    (baseMode as any).abcFiles = new Set();

    (baseMode as any).moduleInfos = new Map();
    (baseMode as any).moduleInfos.set("test-module", {
        packageName: "test-module",
        moduleType: OHOS_MODULE_TYPE.HAR,
        byteCodeHar: true,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "./dist/cache/test/config.json",
        compileFileInfos: []
    });

    (baseMode as any).moduleInfos.set("test-module-2", {
        packageName: "test-module-2",
        moduleType: OHOS_MODULE_TYPE.HAR,
        byteCodeHar: true,
        abcPath: "/test/path/module2.abc",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "./dist/cache/test/config.json",
        compileFileInfos: []
    });

    baseMode.testCollectAbcFileFromByteCodeHar();
    expect((baseMode as any).abcFiles.has("/test/path/module2.abc")).toBe(true);
    expect((baseMode as any).abcFiles.size).toBe(1);

    delete (global as any).LogDataFactory;
    delete (global as any).ErrorCode;
}
*/

function test_collectCompileFiles_enableDeclgenEts2Ts_false() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockConfig = {
        packageName: "test",
        moduleType: OHOS_MODULE_TYPE.HAR,
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        enableDeclgenEts2Ts: false,
        dependencyModuleList: []
    };

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testCollectCompileFiles(): void {
            super.processEntryFiles();
        }
    }

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);

    const baseMode = new TestBaseMode(mockConfig as any);

    baseMode.entryFiles = new Set(['/test/path/file1.ets']);
    baseMode.moduleInfos = new Map();
    baseMode.abcFiles = new Set();
    baseMode.filesHashCache = {};

    baseMode.testCollectCompileFiles();
}

class TestBaseModeMock extends BaseMode {
    public run(): Promise<void> {
        return Promise.resolve();
    }
    public getMainModuleInfo(): ModuleInfo {
        const ARKTSCONFIG_JSON_FILE = 'arktsconfig.json';
        return {
            isMainModule: true,
            packageName: "entry",
            moduleRootPath: "./",
            moduleType: OHOS_MODULE_TYPE.HAR,
            sourceRoots: ["./"],
            entryFile: "index.ets",
            arktsConfigFile: path.resolve(this.cacheDir, "entry", ARKTSCONFIG_JSON_FILE),
            declgenV1OutPath: path.resolve(this.cacheDir, "declgen"),
            declgenV2OutPath: path.resolve(this.cacheDir, "declgen/v2"),
            declgenBridgeCodePath: path.resolve(this.cacheDir, "bridge"),
            dependencies: [],
            dynamicDependencyModules: new Map(),
            staticDependencyModules: new Map(),
            byteCodeHar: false,
        };
    }
    public testCollectModuleInfos(): void {
        return this.collectModuleInfos();
    }
}

function test_collectModuleInfos1(LogDataFactory: LogDataFactory) {
    const mockConfig = mock.getMockedBuildConfig()
    const baseMode = new TestBaseModeMock(mockConfig);
    baseMode.testCollectModuleInfos();
    LogDataFactory.newInstance.mockClear();
}

function test_collectModuleInfos2(LogDataFactory: LogDataFactory) {
    const mockConfig = {
        buildMode: BUILD_MODE.DEBUG,
        compileFiles: ["test.ets"],
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        hasMainModule: true,
        dependencyModuleList: [
            {
                packageName: "dep1",
                sourceRoots: ["./"],
                entryFile: "index.ets"
            }
        ]
    };

    const baseMode = new TestBaseModeMock(mockConfig as any);
    baseMode.testCollectModuleInfos();
    LogDataFactory.newInstance.mockClear();
}

function test_collectModuleInfos3(LogDataFactory: LogDataFactory) {
    const mockConfig = {
        buildMode: BUILD_MODE.DEBUG,
        compileFiles: ["test.ets"],
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        hasMainModule: true,
        dependencyModuleList: [
            {
                packageName: "dep2",
                modulePath: "/test/dep2",
                entryFile: "index.ets"
            }
        ]
    };

    const baseMode = new TestBaseModeMock(mockConfig as any);

    baseMode.testCollectModuleInfos();
    LogDataFactory.newInstance.mockClear();
}

function test_collectModuleInfos4(LogDataFactory: LogDataFactory) {
    const mockConfig = {
        buildMode: BUILD_MODE.DEBUG,
        compileFiles: ["test.ets"],
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        hasMainModule: true,
        dependencyModuleList: [
            {
                packageName: "dep3",
                modulePath: "/test/dep3",
                sourceRoots: ["./"]
            }
        ]
    };

    const baseMode = new TestBaseModeMock(mockConfig as any);
    baseMode.testCollectModuleInfos();
    LogDataFactory.newInstance.mockClear();
}

function test_collectModuleInfos001() {
    const LogDataFactory = { newInstance: jest.fn().mockReturnValue({ code: "123", message: "Test error" }) };

    test_collectModuleInfos1(LogDataFactory);
    test_collectModuleInfos2(LogDataFactory);
    test_collectModuleInfos3(LogDataFactory);
    test_collectModuleInfos4(LogDataFactory);

}

function test_updateDependantJobs1(baseMode: TestBuildMode) {
    const jobId = "job1";
    const processingJobs = new Set<string>([jobId, "job2"]);
    const jobs: Record<string, JobInfo> = {
        "job1": {
            id: "job1",
            jobDependencies: [],
            jobDependants: ["job2", "job3"],
            fileList: ["/test/file1.ets"],
            isAbcJob: true
        },
        "job2": {
            id: "job2",
            jobDependencies: ["job1", "job4"],
            jobDependants: [],
            fileList: ["/test/file2.ets"],
            isAbcJob: true
        },
        "job3": {
            id: "job3",
            jobDependencies: ["job1"],
            jobDependants: [],
            fileList: ["/test/file3.ets"],
            isAbcJob: true
        }
    };

    baseMode.testUpdateDependantJobs(jobId, processingJobs, jobs);

    expect(processingJobs.has(jobId)).toBe(false);
    expect(jobs["job2"].jobDependencies).not.toContain("job1");
    expect(jobs["job2"].jobDependencies).toContain("job4");
    expect(jobs["job3"].jobDependencies.length).toBe(0);
    expect(baseMode.addJobToQueues).toHaveBeenCalledWith(jobs["job3"]);
}

function test_updateDependantJobs2(global: any, baseMode: any) {
    (global as any).finishedJob = [];
    (baseMode as any).addJobToQueues.mockClear();

    const jobId = "job5";
    const processingJobs = new Set<string>([jobId]);
    const jobs: Record<string, JobInfo> = {
        "job5": {
            id: "job5",
            jobDependencies: [],
            jobDependants: ["job6", "nonExistingJob"],
            fileList: ["/test/file5.ets"],
            isAbcJob: true
        },
        "job6": {
            id: "job6",
            jobDependencies: ["job5"],
            jobDependants: [],
            fileList: ["/test/file6.ets"],
            isAbcJob: true
        }
    };

    baseMode.testUpdateDependantJobs(jobId, processingJobs, jobs);

    expect(processingJobs.has(jobId)).toBe(false);
    expect((baseMode as any).addJobToQueues).toHaveBeenCalledWith(jobs["job6"]);
}

function test_updateDependantJobs3(global: any, baseMode: any) {
    (global as any).finishedJob = [];
    (baseMode as any).addJobToQueues.mockClear();

    const jobId = "job7";
    const processingJobs = new Set<string>([jobId]);
    const jobs: Record<string, JobInfo> = {
        "job7": {
            id: "job7",
            jobDependencies: [],
            jobDependants: ["job8"],
            fileList: ["/test/file7.ets"],
            isAbcJob: true
        },
        "job8": {
            id: "job8",
            jobDependencies: ["job9"],
            jobDependants: [],
            fileList: ["/test/file8.ets"],
            isAbcJob: true
        }
    };

    baseMode.testUpdateDependantJobs(jobId, processingJobs, jobs);

    expect(jobs["job8"].jobDependencies).toEqual(["job9"]);
    expect((baseMode as any).addJobToQueues).not.toHaveBeenCalled();
}

function test_updateDependantJobs() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };
    global.finishedJob = [];
    class TestBuildMode extends BuildMode {
        public testUpdateDependantJobs(jobId: string, processingJobs: Set<string>, jobs: Record<string, JobInfo>): void {
            return this.updateDependantJobs(jobId, processingJobs, jobs);
        }
    }
    const baseMode = new TestBuildMode(mockConfig as any);
    (baseMode as any).addJobToQueues = jest.fn();
    test_updateDependantJobs1(baseMode);
    test_updateDependantJobs2(global as any, baseMode as any);
    test_updateDependantJobs3(global as any, baseMode as any);

    delete (global as any).finishedJob;
}

function test_loadHashCache() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    const fs = require('fs');
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockLogData = { code: "123", message: "Test error" };

    const LogDataFactory = {
        newInstance: jest.fn().mockReturnValue(mockLogData)
    };

    const ErrorCode = {
        BUILDSYSTEM_LOAD_HASH_CACHE_FAIL: '11410100'
    };

    class TestBuildMode extends BuildMode {
        public testLoadHashCache(): Record<string, string> {
            return (this as any).loadHashCache();
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);
    (baseMode as any).logger = mockLogger;
    (baseMode as any).hashCacheFile = "/test/cache/hash_cache.json";

    (global as any).LogDataFactory = LogDataFactory;
    (global as any).ErrorCode = ErrorCode;

    jest.spyOn(fs, 'existsSync').mockReturnValueOnce(false);
    let result = baseMode.testLoadHashCache();
    expect(result).toEqual({});

    (baseMode as any).entryFiles = new Set(['file1.ets', 'file2.ets']);
    jest.spyOn(fs, 'existsSync').mockReturnValueOnce(true);
    jest.spyOn(fs, 'readFileSync').mockReturnValueOnce('{"file1.ets":"hash1","file2.ets":"hash2"}');
    result = baseMode.testLoadHashCache();
    expect(result).toEqual({
        "file1.ets": "hash1",
        "file2.ets": "hash2"
    });

    jest.spyOn(fs, 'existsSync').mockReturnValueOnce(true);
    jest.spyOn(fs, 'readFileSync').mockImplementationOnce(() => {
        throw new Error("File read error");
    });
    result = baseMode.testLoadHashCache();
    expect(result).toEqual({});

    delete (global as any).LogDataFactory;
    delete (global as any).ErrorCode;
    jest.restoreAllMocks();
}

function test_isFileChanged() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBuildMode extends BuildMode {
        public testIsFileChanged(etsFilePath: string, abcFilePath: string): boolean {
            return (this as any).isFileChanged(etsFilePath, abcFilePath);
        }
    }

    const fs = require('fs');
    const existsSyncSpy = jest.spyOn(fs, 'existsSync');
    const statSyncSpy = jest.spyOn(fs, 'statSync');

    (global as any).getFileHash = jest.fn();

    const baseMode = new TestBuildMode(mockConfig as any);
    (baseMode as any).hashCache = {};

    existsSyncSpy.mockReturnValueOnce(false);
    let result = baseMode.testIsFileChanged('/test/file1.ets', '/test/file1.abc');
    expect(result).toBe(true);
    expect(existsSyncSpy).toHaveBeenCalledWith('/test/file1.abc');
    expect(statSyncSpy).not.toHaveBeenCalled();

    existsSyncSpy.mockReturnValueOnce(true);
    statSyncSpy.mockReturnValueOnce({ mtimeMs: 200 });
    statSyncSpy.mockReturnValueOnce({ mtimeMs: 100 });
    result = baseMode.testIsFileChanged('/test/file2.ets', '/test/file2.abc');
    expect(result).toBe(true);
    expect(statSyncSpy).toHaveBeenCalledWith('/test/file2.ets');
    expect(statSyncSpy).toHaveBeenCalledWith('/test/file2.abc');

    jest.restoreAllMocks();
    delete (global as any).getFileHash;
}

function test_collectDependentCompileFiles() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const LogDataFactory = {
        newInstance: jest.fn().mockReturnValue({ code: "123", message: "Test error" })
    };

    const ErrorCode = {
        BUILDSYSTEM_Dependency_Analyze_FAIL: '11410001',
        BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL: '11410002'
    };

    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBuildMode extends BuildMode {
        public testCollectDependentCompileFiles(): void {
            return (this as any).collectDependentCompileFiles();
        }
    }

    (global as any).LogDataFactory = LogDataFactory;
    (global as any).ErrorCode = ErrorCode;
    (global as any).getFileHash = jest.fn().mockReturnValue("hash123");

    const baseMode = new TestBuildMode(mockConfig as any);
    (baseMode as any).logger = mockLogger;
    (baseMode as any).cacheDir = "/test/cache";
    (baseMode as any).hashCache = {};
    (baseMode as any).abcFiles = new Set();
    (baseMode as any).allFiles = new Map();
    (baseMode as any).compileFiles = new Map();

    {
        (baseMode as any).dependencyFileMap = null;

        baseMode.testCollectDependentCompileFiles();

        mockLogger.printError.mockClear();
        LogDataFactory.newInstance.mockClear();
    }

    {
        (baseMode as any).dependencyFileMap = {
            dependants: {
                "/test/other/path/file.ets": []
            }
        };
        (baseMode as any).entryFiles = new Set(["/test/other/path/file.ets"]);
        (baseMode as any).moduleInfos = new Map([
            ["test", {
                packageName: "test",
                moduleRootPath: "/test/path",
                sourceRoots: ["./"]
            }]
        ]);

        baseMode.testCollectDependentCompileFiles();

        mockLogger.printError.mockClear();
        LogDataFactory.newInstance.mockClear();
    }

    delete (global as any).LogDataFactory;
    delete (global as any).ErrorCode;
    delete (global as any).getFileHash;
    jest.restoreAllMocks();
}

function test_getSerializableConfig() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG,
        arkts: {
            someFunction: () => { }
        },
        bigIntValue: BigInt(9007199254740991)
    };

    class TestBuildMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testGetSerializableConfig(): Object {
            return (this as any).getSerializableConfig();
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);

    const result = baseMode.testGetSerializableConfig();

    expect(result).not.toHaveProperty('arkts');

    expect(result).not.toHaveProperty('bigIntValue');

    expect(result).toHaveProperty('packageName', 'test');
    expect(result).toHaveProperty('moduleRootPath', '/test/path');
    expect(result).toHaveProperty('sourceRoots');
}

function test_getJobDependencies() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBuildMode extends BuildMode {
        public testGetJobDependencies(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
            return (this as any).getJobDependencies(fileDeps, cycleFiles);
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);
    {
        const fileDeps = ['/test/path/file1.ets', '/test/path/file2.ets'];
        const cycleFiles = new Map<string, string[]>();
        const result = baseMode.testGetJobDependencies(fileDeps, cycleFiles);
        expect(result.size).toBe(2);
        expect(result.has('0/test/path/file1.ets')).toBe(true);
        expect(result.has('0/test/path/file2.ets')).toBe(true);
    }

    {
        const fileDeps = ['/test/path/file1.ets', '/test/path/cycle1.ets'];
        const cycleFiles = new Map<string, string[]>();
        cycleFiles.set('/test/path/cycle1.ets', ['cycle-group-1', 'cycle-group-2']);
        const result = baseMode.testGetJobDependencies(fileDeps, cycleFiles);
        expect(result.size).toBe(3);
        expect(result.has('0/test/path/file1.ets')).toBe(true);
        expect(result.has('cycle-group-1')).toBe(true);
        expect(result.has('cycle-group-2')).toBe(true);
        expect(result.has('0/test/path/cycle1.ets')).toBe(false);
    }

    {
        const fileDeps = ['/test/path/cycle1.ets', '/test/path/cycle2.ets'];
        const cycleFiles = new Map<string, string[]>();
        cycleFiles.set('/test/path/cycle1.ets', ['cycle-group-1']);
        cycleFiles.set('/test/path/cycle2.ets', ['cycle-group-2']);
        const result = baseMode.testGetJobDependencies(fileDeps, cycleFiles);
        expect(result.size).toBe(2);
        expect(result.has('cycle-group-1')).toBe(true);
        expect(result.has('cycle-group-2')).toBe(true);
        expect(result.has('0/test/path/cycle1.ets')).toBe(false);
        expect(result.has('0/test/path/cycle2.ets')).toBe(false);
    }
}

function test_getJobDependants() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };
    class TestBuildMode extends BuildMode {
        public testGetJobDependants(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
            return (this as any).getJobDependants(fileDeps, cycleFiles);
        }
    }
    const baseMode = new TestBuildMode(mockConfig as any);
    {
        const fileDeps = ['/test/path/file1.ets', '/test/path/file2.ets'];
        const cycleFiles = new Map<string, string[]>();
        const result = baseMode.testGetJobDependants(fileDeps, cycleFiles);
        expect(result.size).toBe(4);
        expect(result.has('1/test/path/file1.ets')).toBe(true);
        expect(result.has('0/test/path/file1.ets')).toBe(true);
        expect(result.has('1/test/path/file2.ets')).toBe(true);
        expect(result.has('0/test/path/file2.ets')).toBe(true);
    }

    {
        const fileDeps = ['/test/path/file1.d.ets', '/test/path/file2.ets'];
        const cycleFiles = new Map<string, string[]>();
        const result = baseMode.testGetJobDependants(fileDeps, cycleFiles);
        expect(result.size).toBe(3);
        expect(result.has('1/test/path/file1.d.ets')).toBe(false);
        expect(result.has('0/test/path/file1.d.ets')).toBe(true);
        expect(result.has('1/test/path/file2.ets')).toBe(true);
        expect(result.has('0/test/path/file2.ets')).toBe(true);
    }

    {
        const fileDeps = ['/test/path/file1.ets', '/test/path/cycle1.ets'];
        const cycleFiles = new Map<string, string[]>();
        cycleFiles.set('/test/path/cycle1.ets', ['cycle-group-1', 'cycle-group-2']);
        const result = baseMode.testGetJobDependants(fileDeps, cycleFiles);
        expect(result.size).toBe(5);
        expect(result.has('1/test/path/file1.ets')).toBe(true);
        expect(result.has('0/test/path/file1.ets')).toBe(true);
        expect(result.has('cycle-group-1')).toBe(true);
        expect(result.has('cycle-group-2')).toBe(true);
        expect(result.has('0/test/path/cycle1.ets')).toBe(false);
    }
}

// NOTE: move to Dependency Analyzer ut
/*
function test_collectCompileJobs() {
    const mockConfig = getMockConfig()

    class TestBuildMode extends BuildMode {
        public testCollectCompileJobs(jobs: Record<string, Job>): void {
            return (this as any).collectCompileJobs(jobs);
        }

        constructor(buildConfig: any) {
            super(buildConfig);
            this.dependencyFileMap = {
                dependencies: {
                    '/test/path/file1.ets': ['/test/path/file2.ets'],
                    '/test/path/file3.ets': ['/test/path/file4.ets'],
                    '/test/path/file5.d.ets': []
                },
                dependants: {
                    '/test/path/file2.ets': ['/test/path/file1.ets'],
                    '/test/path/file4.ets': ['/test/path/file3.ets']
                }
            };

            this.entryFiles = new Set(['/test/path/file1.ets']);
            this.compileFiles = new Map([
                ['/test/path/file1.ets', { filePath: '/test/path/file1.ets' }]
            ]);

            this.moduleInfos = new Map();
            this.moduleInfos.set("test", {
                packageName: "test",
                moduleRootPath: "/test/path",
                arktsConfigFile: "/test/path/config.json"
            };

            this.allFiles = new Map();

            this.getJobDependencies = jest.fn().mockImplementation(() => new Set(['dep1', 'dep2']));
            this.getJobDependants = jest.fn().mockImplementation(() => new Set(['dep3', 'dep4']));
            this.getAbcJobId = jest.fn().mockImplementation((file) => '1' + file);
            this.getExternalProgramJobId = jest.fn().mockImplementation((file) => '0' + file);
            this.createExternalProgramJob = jest.fn();
            this.dealWithDependants = jest.fn();
            this.findStronglyConnectedComponents = jest.fn().mockImplementation(() => {
                const cycleGroups = new Map();
                const cycle1 = new Set(['/test/path/cycle1.ets', '/test/path/cycle2.ets']);
                cycleGroups.set('cycle-group-1', cycle1);
                return cycleGroups;
            });
        }
    }

    const baseMode = new TestBuildMode(mockConfig);

    (baseMode as any).dependencyFileMap.dependants['/test/path/file6.ets'] = ['/test/path/file7.ets'];

    const jobs: Record<string, Job> = {};

    const findComponentsSpy = jest.spyOn(baseMode as any, 'findStronglyConnectedComponents');
    const getJobDependenciesSpy = jest.spyOn(baseMode as any, 'getJobDependencies');
    const getJobDependantsSpy = jest.spyOn(baseMode as any, 'getJobDependants');
    const getAbcJobIdSpy = jest.spyOn(baseMode as any, 'getAbcJobId');
    const getExternalProgramJobIdSpy = jest.spyOn(baseMode as any, 'getExternalProgramJobId');
    const createExternalProgramJobSpy = jest.spyOn(baseMode as any, 'createExternalProgramJob');
    const dealWithDependantsSpy = jest.spyOn(baseMode as any, 'dealWithDependants');

    baseMode.testCollectCompileJobs(jobs);

    expect((baseMode as any).dependencyFileMap.dependencies['/test/path/file6.ets']).toEqual([]);

    expect(findComponentsSpy).toHaveBeenCalledWith((baseMode as any).dependencyFileMap);

    const cycleFiles = new Map();
    cycleFiles.set('/test/path/cycle1.ets', ['cycle-group-1']);
    cycleFiles.set('/test/path/cycle2.ets', ['cycle-group-1']);

    expect(getJobDependenciesSpy).toHaveBeenCalled();

    expect(getAbcJobIdSpy).toHaveBeenCalledWith('/test/path/file1.ets');
    expect(getAbcJobIdSpy).toHaveBeenCalledWith('/test/path/file3.ets');
    expect(getAbcJobIdSpy).not.toHaveBeenCalledWith('/test/path/file5.d.ets');

    expect(jobs['1/test/path/file1.ets']).toBeDefined();
    expect(jobs['1/test/path/file3.ets']).toBeDefined();

    expect(createExternalProgramJobSpy).toHaveBeenCalled();

    expect((baseMode as any).allFiles.has('/test/path/file5.d.ets')).toBe(true);

    expect(getJobDependantsSpy).toHaveBeenCalled();
    expect(dealWithDependantsSpy).toHaveBeenCalled();

    jest.restoreAllMocks();
}
*/

function test_dealWithDependants() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };
    class TestBuildMode extends BuildMode {
        public testDealWithDependants(cycleFiles: Map<string, string[]>, key: string, jobs: Record<string, JobInfo>, dependants: Set<string>): void {
            return (this as any).dealWithDependants(cycleFiles, key, jobs, dependants);
        }
    }
    const baseMode = new TestBuildMode(mockConfig as any);
    {
        const cycleFiles = new Map<string, string[]>();
        cycleFiles.set('file1.ets', ['cycle-1', 'cycle-2']);
        const jobs: Record<string, JobInfo> = {
            'cycle-1': {
                id: 'cycle-1',
                fileList: ['file1.ets'],
                jobDependencies: [],
                jobDependants: ['dep1', 'dep2'],
                isAbcJob: false
            },
            'cycle-2': {
                id: 'cycle-2',
                fileList: ['file1.ets', 'file2.ets'],
                jobDependencies: [],
                jobDependants: ['dep3'],
                isAbcJob: false
            }
        };
        const dependants = new Set<string>(['dep4', 'dep5', 'cycle-1']);
        baseMode.testDealWithDependants(cycleFiles, 'file1.ets', jobs, dependants);
        expect(jobs['cycle-1'].jobDependants).toEqual(expect.arrayContaining(['dep1', 'dep2', 'dep4', 'dep5']));
        expect(jobs['cycle-1'].jobDependants).not.toContain('cycle-1');
        expect(jobs['cycle-2'].jobDependants).toEqual(expect.arrayContaining(['dep3', 'dep4', 'dep5']));
        expect(jobs['cycle-2'].jobDependants).not.toContain('cycle-1');
    }
    {
        const cycleFiles = new Map<string, string[]>();
        const jobs: Record<string, JobInfo> = {
            '0file2.ets': {
                id: '0file2.ets',
                fileList: ['file2.ets'],
                jobDependencies: [],
                jobDependants: ['dep1', 'dep2'],
                isAbcJob: false
            }
        };
        const dependants = new Set<string>(['dep3', 'dep4', '0file2.ets']);
        baseMode.testDealWithDependants(cycleFiles, 'file2.ets', jobs, dependants);
        expect(jobs['0file2.ets'].jobDependants).toEqual(expect.arrayContaining(['dep1', 'dep2', 'dep3', 'dep4']));
        expect(jobs['0file2.ets'].jobDependants).not.toContain('0file2.ets');
    }
}

function test_addJobToQueues() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBuildMode extends BuildMode {
        public testAddJobToQueues(job: JobInfo): void {
            return this.consumeJob(job);
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);

    const job1: JobInfo = {
        id: 'job1',
        fileList: ['/test/path/file1.ets'],
        jobDependencies: [],
        jobDependants: [],
        isAbcJob: false
    };
    baseMode.testAddJobToQueues(job1);
    expect(queues1.externalProgramQueue.length).toBe(1);
    expect(queues1.externalProgramQueue[0].id).toBe('job1');
    expect(queues1.abcQueue.length).toBe(0);

    const job2: JobInfo = {
        id: 'job2',
        fileList: ['/test/path/file2.ets'],
        jobDependencies: [],
        jobDependants: [],
        isAbcJob: true
    };
    baseMode.testAddJobToQueues(job2);
    expect(queues2.externalProgramQueue.length).toBe(0);
    expect(queues2.abcQueue.length).toBe(1);
    expect(queues2.abcQueue[0].id).toBe('job2');

    const job3: JobInfo = {
        id: 'job3',
        fileList: ['/test/path/file3.ets'],
        jobDependencies: [],
        jobDependants: [],
        isAbcJob: false
    };
    baseMode.testAddJobToQueues(job3, queues3);
    expect(queues3.externalProgramQueue.length).toBe(1);
    expect(queues3.abcQueue.length).toBe(0);

    const job4: JobInfo = {
        id: 'job4',
        fileList: ['/test/path/file4.ets'],
        jobDependencies: [],
        jobDependants: [],
        isAbcJob: true
    };
    baseMode.testAddJobToQueues(job4, queues4);
    expect(queues4.externalProgramQueue.length).toBe(0);
    expect(queues4.abcQueue.length).toBe(1);
}

function test_initCompileQueues() {
    const mockConfig = mock.getMockedBuildConfig()
    class TestBuildMode extends BuildMode {
        public testInitCompileQueues(jobs: Record<string, JobInfo>, queues: Queues): void {
            return (this as any).initCompileQueues(jobs, queues);
        }

        constructor(buildConfig: any) {
            super(buildConfig);
            this.collectCompileJobs = jest.fn().mockImplementation((jobs: Record<string, JobInfo>) => {
                jobs['job1'] = {
                    id: 'job1',
                    jobDependencies: [],
                    jobDependants: ['job3'],
                    fileList: ['/test/path/file1.ets'],
                    isAbcJob: true,
                };

                jobs['job2'] = {
                    id: 'job2',
                    jobDependencies: [],
                    jobDependants: [],
                    fileList: ['/test/path/file2.ets'],
                    isAbcJob: false,
                };

                jobs['job3'] = {
                    id: 'job3',
                    jobDependencies: ['job1'],
                    jobDependants: [],
                    fileList: ['/test/path/file3.ets'],
                    isAbcJob: true,
                };
            });

            (this as any).addJobToQueues = jest.fn().mockImplementation((job: JobInfo, queues: Queues) => {
                if (job.isAbcJob) {
                    queues.abcQueue.push(job);
                } else {
                    queues.externalProgramQueue.push(job);
                }
            });
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);

    const jobs: Record<string, JobInfo> = {};
    const collectCompileJobsSpy = jest.spyOn(baseMode, 'collectCompileJobs');
    const addJobToQueuesSpy = jest.spyOn(baseMode, 'addJobToQueues');

    baseMode.testInitCompileQueues(jobs, queues);

    expect(collectCompileJobsSpy).toHaveBeenCalledWith(jobs);

    expect(addJobToQueuesSpy).toHaveBeenCalledTimes(2);

    expect(queues.abcQueue.length).toBe(1);
    expect(queues.abcQueue[0].id).toBe('job1');
    expect(queues.externalProgramQueue.length).toBe(1);
    expect(queues.externalProgramQueue[0].id).toBe('job2');

    expect(queues.abcQueue.find((job: JobInfo) => job.id === 'job3')).toBeUndefined();

    jest.restoreAllMocks();
}


// NOTE: To be defined later
/*
function test_checkAllTasksDone() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };

    class TestBuildMode extends BuildMode {
        public testCheckAllTasksDone(queues: Queues, workerPool: WorkerInfo[]): boolean {
            return (this as any).checkAllTasksDone(queues, workerPool);
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);
    const queues2: Queues = {
        externalProgramQueue: [],
        abcQueue: []
    };
    const workerPool2 = [
        { worker: {} as ThreadWorker, isIdle: true },
        { worker: {} as ThreadWorker, isIdle: false }
    ];

    expect(baseMode.testCheckAllTasksDone(queues2, workerPool2)).toBe(false);
    const queues3: Queues = {
        externalProgramQueue: [],
        abcQueue: []
    };
    const workerPool3 = [
        { worker: {} as ThreadWorker, isIdle: true },
        { worker: {} as ThreadWorker, isIdle: true }
    ];
    expect(baseMode.testCheckAllTasksDone(queues3, workerPool3)).toBe(true);
    expect(baseMode.testCheckAllTasksDone(queues3, workerPool3)).toBe(true);
}

function test_processAfterCompile() {
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG,
        arkts: {
            destroyConfig: jest.fn()
        },
        arktsGlobal: {
            es2panda: {
                _DestroyGlobalContext: jest.fn(),
                _MemFinalize: jest.fn()
            }
        }
    };

    class TestBuildMode extends BuildMode {
        public testProcessAfterCompile(config: any, globalContext: any): void {
            (this as any).processAfterCompile(config, globalContext);
        }

        public mergeAbcFiles(): void {
        }
    }

    const baseMode = new TestBuildMode(mockConfig as any);
    const mergeAbcFilesSpy = jest.spyOn(baseMode, 'mergeAbcFiles').mockImplementation(() => { });

    baseMode.testProcessAfterCompile('mockConfig', 'mockGlobalContext');
    expect(mockConfig.arktsGlobal.es2panda._DestroyGlobalContext).toHaveBeenCalledWith('mockGlobalContext');
    expect(mockConfig.arkts.destroyConfig).toHaveBeenCalledWith('mockConfig');
    expect(mockConfig.arktsGlobal.es2panda._MemFinalize).toHaveBeenCalled();
    expect(mergeAbcFilesSpy).toHaveBeenCalledTimes(1);
    expect((baseMode as any).hasCleanWorker).toBe(true);

    jest.clearAllMocks();
    baseMode.testProcessAfterCompile('mockConfig2', 'mockGlobalContext2');
    expect(mockConfig.arktsGlobal.es2panda._DestroyGlobalContext).not.toHaveBeenCalled();
    expect(mockConfig.arkts.destroyConfig).not.toHaveBeenCalled();
    expect(mockConfig.arktsGlobal.es2panda._MemFinalize).not.toHaveBeenCalled();
    expect(mergeAbcFilesSpy).not.toHaveBeenCalled();

    jest.restoreAllMocks();
}

function test_runConcurrent() {
    const mockConfig = {
        packageName: "test",
        compileFiles: ["/test/path/file1.ets"],
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    } as any;

    const Logger = require('../../../src/logger').Logger;
    Logger.getInstance = jest.fn().mockReturnValue({
        printInfo: jest.fn(), printError: jest.fn(), hasErrors: jest.fn().mockReturnValue(false)
    });

    class TestBuildMode extends BuildMode {
        public async testRunConcurrent(): Promise<void> { return this.runConcurrent(); }

        public generateModuleInfos(): void {
            (this as any).compileFiles = new Map([
                ['/test/path/file1.ets', {
                    filePath: '/test/path/file1.ets', packageName: 'test',
                    abcFilePath: '/test/path/output.abc', arktsConfigFile: '/test/arktsconfig.json'
                }]
            ]);
            (this as any).allFiles = (this as any).compileFiles;
        }
        public generateArkTSConfigForModules(): void { }

        constructor(buildConfig: any) {
            super(buildConfig);
            const self = this as any;
            self.initCompileQueues = function(jobs: any, queues: any): void {
                queues.externalProgramQueue.push({
                    id: '0/test/path/file1.ets', fileList: ['/test/path/file1.ets'],
                    dependencies: [], dependants: [], isDeclFile: true, isAbcJob: false
                });
            };
            self.invokeWorkers = async function(): Promise<void> { return Promise.resolve(); };
        }
    }

    const baseMode = new TestBuildMode(mockConfig);
    const genModuleSpy = jest.spyOn(baseMode, 'generateModuleInfos');
    const genConfigSpy = jest.spyOn(baseMode, 'generateArkTSConfigForModules');
    const initQueuesSpy = jest.spyOn(baseMode, 'initCompileQueues' as any);
    const invokeWorkersSpy = jest.spyOn(baseMode, 'invokeWorkers' as any);
    return baseMode.testRunConcurrent().then(() => {
        expect(genModuleSpy).toHaveBeenCalledTimes(1);
        expect(genConfigSpy).toHaveBeenCalledTimes(1);
        expect(initQueuesSpy).toHaveBeenCalledTimes(1);
        expect(invokeWorkersSpy).toHaveBeenCalledTimes(1);
        jest.restoreAllMocks();
    });
}
*/

function test_collectDependencyModules_language_branches() {
    class TestBaseMode extends BaseMode {
        public run(): Promise<void> { return Promise.resolve(); }
        public testCollectDependencyModules(
            packageName: string, module: ModuleInfo,
            dynamicDepModules: Map<string, ModuleInfo>,
            staticDepModules: Map<string, ModuleInfo>
        ): void {
            (this as any).collectDependencyModules(packageName, module, dynamicDepModules, staticDepModules);
        }
    }

    const baseMode = new TestBaseMode({
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    } as any);

    {
        const packageName = "mod_1_1";
        const module = { packageName: "mod_1_1", language: LANGUAGE_VERSION.ARKTS_1_1 } as ModuleInfo;
        const dynamicDepModules = new Map<string, ModuleInfo>();
        const staticDepModules = new Map<string, ModuleInfo>();

        baseMode.testCollectDependencyModules(packageName, module, dynamicDepModules, staticDepModules);

        expect(dynamicDepModules.has(packageName)).toBe(true);
        expect(staticDepModules.has(packageName)).toBe(false);
        expect(dynamicDepModules.get(packageName)).toBe(module);
    }

    {
        const packageName = "mod_1_2";
        const module = { packageName: "mod_1_2", language: LANGUAGE_VERSION.ARKTS_1_2 } as ModuleInfo;
        const dynamicDepModules = new Map<string, ModuleInfo>();
        const staticDepModules = new Map<string, ModuleInfo>();

        baseMode.testCollectDependencyModules(packageName, module, dynamicDepModules, staticDepModules);

        expect(staticDepModules.has(packageName)).toBe(true);
    }

    {
        const packageName = "mod_hybrid";
        const module = { packageName: "mod_hybrid", language: LANGUAGE_VERSION.ARKTS_HYBRID } as ModuleInfo;
        const dynamicDepModules = new Map<string, ModuleInfo>();
        const staticDepModules = new Map<string, ModuleInfo>();

        baseMode.testCollectDependencyModules(packageName, module, dynamicDepModules, staticDepModules);

        expect(dynamicDepModules.has(packageName)).toBe(true);
        expect(staticDepModules.has(packageName)).toBe(true);
        expect(dynamicDepModules.get(packageName)).toBe(module);
        expect(staticDepModules.get(packageName)).toBe(module);
    }
}

function test_getDependentModules_missing_module() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn()
    };
    const ErrorCode = {
        BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_FOUND: 'BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_FOUND'
    };
    jest.mock('../../../src/util/error', () => ({
        ErrorCode
    }));
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        buildMode: BUILD_MODE.DEBUG
    };
    const Logger = require('../../../src/logger').Logger;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);
    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }
        public testGetDependentModules(moduleInfo: ModuleInfo): Map<string, ModuleInfo>[] {
            return (this as any).getDependentModules(moduleInfo);
        }
    }
    const baseMode = new TestBaseMode(mockConfig as any);
    (baseMode as any).logger = mockLogger;
    const testModuleInfo = {
        isMainModule: false,
        dependencies: ['nonExistingModule'],
        packageName: 'testModule'
    } as ModuleInfo;
    baseMode.testGetDependentModules(testModuleInfo);
    expect(mockLogger.printErrorAndExit).toHaveBeenCalledWith(
        expect.objectContaining({
            cause: "",
            code: "11410011",
            description: 'Module nonExistingModule not found in moduleInfos'
        })
    );
}

// NOTE: to be defined later
/*
function test_declgen_method() {
    jest.resetAllMocks();
    jest.restoreAllMocks();
    const fs = require('fs');
    jest.spyOn(fs, 'readFileSync').mockReturnValue('test source code');
    const mockConfig = {
        packageName: "test",
        moduleRootPath: "/test/path",
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
        arkts: {
            Config: { create: jest.fn().mockReturnValue({ peer: 'mockConfigPeer' }) },
            Context: {
                createFromString: jest.fn().mockReturnValue({ peer: 'mockContextPeer', program: 'mockProgram' }),
                createFromStringWithHistory: jest.fn().mockReturnValue({ peer: 'mockContextPeer', program: 'mockProgram' })
            },
            proceedToState: jest.fn(), EtsScript: { fromContext: jest.fn().mockReturnValue('mockAst') },
            Es2pandaContextState: { ES2PANDA_STATE_PARSED: 'parsed', ES2PANDA_STATE_CHECKED: 'checked' },
            generateTsDeclarationsFromContext: jest.fn(), destroyConfig: jest.fn()
        },
        arktsGlobal: { es2panda: { _DestroyContext: jest.fn() } }
    };
    const PluginDriver = require('../../../src/plugins/plugins_driver').PluginDriver;
    PluginDriver.getInstance = jest.fn().mockReturnValue({
        getPluginContext: jest.fn().mockReturnValue({ setArkTSProgram: jest.fn(), setArkTSAst: jest.fn() }),
        runPluginHook: jest.fn()
    });
    jest.spyOn(utils, 'ensurePathExists').mockImplementation(() => { });
    jest.spyOn(utils, 'changeDeclgenFileExtension').mockReturnValueOnce('/test/path/output.d.ets').mockReturnValueOnce('/test/path/output.ts');
    jest.spyOn(path, 'relative').mockReturnValue('file1.ets');
    jest.spyOn(path, 'join').mockReturnValue('/test/path/output');
    class TestBuildMode extends BuildMode {
        constructor(buildConfig: any) {
            super(buildConfig);
            (this as any).outputDir = './dist'; (this as any).cacheDir = './dist/cache';
        }
        public testDeclgen(fileInfo: any): void { return this.declgen(fileInfo); }
    }
    const baseMode = new TestBuildMode(mockConfig as any);
    (baseMode as any).logger = Logger.getInstance();
    (baseMode as any).moduleInfos = new Map([['test', {
        packageName: 'test', moduleRootPath: '/test/path',
        declgenV1OutPath: './dist/declgen', declgenBridgeCodePath: './dist/bridge'
    }]]);
    baseMode.testDeclgen({ filePath: '/test/path/file1.ets', packageName: 'test', arktsConfigFile: '/test/path/arktsconfig.json' });
    expect(fs.readFileSync).toHaveBeenCalledWith('/test/path/file1.ets', 'utf8');
    expect(mockConfig.arkts.Context.createFromStringWithHistory).toHaveBeenCalled();
    expect(mockConfig.arkts.proceedToState).toHaveBeenCalledWith('parsed', 'mockContextPeer', true);
    expect(mockConfig.arkts.proceedToState).toHaveBeenCalledWith('checked', 'mockContextPeer', true);
    expect(mockConfig.arkts.generateTsDeclarationsFromContext).toHaveBeenCalled();
    jest.restoreAllMocks();
}
*/

// NOTE: to be defined later
/*
function test_generateDeclaration() {
    const mockConfig: BuildConfig = {
        buildMode: BUILD_MODE.DEBUG,
        compileFiles: ["ets2panda/driver/build_system/test/ut/mock/a.ets"],
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        plugins: {},
        dependencyModuleList: [],
        buildType: BUILD_TYPE.BUILD,
        hasMainModule: false,
        moduleType: OHOS_MODULE_TYPE.HAR,
        byteCodeHar: false,
        arkts: {} as any,
        arktsGlobal: {} as any,
        declgenV1OutPath: "./dist/declgen",
        declgenV2OutPath: "./dist/declgen/v2",
        buildSdkPath: "./sdk",
        externalApiPaths: [],
        enableDeclgenEts2Ts: false
    } as any;

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance(mockConfig);

    class TestBuildMode extends BuildMode {
        public async testGenerateDeclaration(): Promise<void> {
            return this.generateDeclaration();
        }

        public generateModuleInfos(): void {
        }

        public declgen(fileInfo: any): void {
        }
    }

    const baseMode = new TestBuildMode(mockConfig);

    (baseMode as any).logger = { printInfo: jest.fn(), printError: jest.fn() };

    const generateModuleInfosSpy = jest.spyOn(baseMode, 'generateModuleInfos').mockImplementation(() => { });
    const declgenSpy = jest.spyOn(baseMode, 'declgen').mockImplementation(() => { });

    return baseMode.testGenerateDeclaration().then(() => {
        expect(generateModuleInfosSpy).toHaveBeenCalledTimes(1);
        generateModuleInfosSpy.mockRestore();
        declgenSpy.mockRestore();
    });
}
*/

async function test_runMethod() {
    const mockConfig: BuildConfig = mock.getMockedBuildConfig()
    mockConfig.compileFiles.push("/test/dependency/path/index.ets")
    mockConfig.declgenV1OutPath = "./dist/declgen"
    mockConfig.declgenV2OutPath = "./dist/declgen/v2"
    mockConfig.dependencyModuleList.push({
        packageName: "testDependency",
        moduleName: "testDependency",
        moduleType: OHOS_MODULE_TYPE.HAR,
        modulePath: "/test/dependency/path",
        sourceRoots: ["./"],
        entryFile: "index.ets",
        language: "1.2"
    })

    class TestBuildMode extends BuildMode {
        public run(): Promise<void> {
            return super.run();
        }

        public collectModuleInfos() {
            super.collectModuleInfos()
        }
    }

    const testBuildMode = new TestBuildMode(mockConfig);
    testBuildMode.koalaModule = {
        arkts: {
            compiler: '/path/to/compiler',
            args: [],
            destroyConfig: jest.fn()
        } as any,
        arktsGlobal: {
            config: {}
        } as any
    } as any

    const mainModuleInfo: ModuleInfo = getMockMainModuleInfo()

    const dependencyModuleInfo: ModuleInfo = {
        isMainModule: false,
        packageName: "dependency",
        moduleRootPath: "/test/dependency/path",
        moduleType: OHOS_MODULE_TYPE.HAR,
        sourceRoots: ["./"],
        entryFile: "index.ets",

        arktsConfigFile: "/dist/cache/test/dependency/arktsconfig.json",
        dependencies: [],
        staticDependencyModules: new Map(),
        dynamicDependencyModules: new Map(),
        language: LANGUAGE_VERSION.ARKTS_1_1,
    }
    mainModuleInfo.dependencies.push("dependency")
    mainModuleInfo.staticDependencyModules.set("dependency", dependencyModuleInfo)


    const generateModuleInfosSpy = jest.spyOn(testBuildMode, 'collectModuleInfos')
        .mockImplementation(() => {
            testBuildMode.fileToModule = new Map([
                ['/test/path/file1.ets', mainModuleInfo],
                ['/test/path/file2.ets', dependencyModuleInfo]
            ]);
            return;
        });

    return testBuildMode.run().then(() => {
        expect(generateModuleInfosSpy).toHaveBeenCalledTimes(1);
        generateModuleInfosSpy.mockRestore();
    });
}


function test_findStronglyConnectedComponents_branches() {
    const mockConfig: BuildConfig = mock.getMockedBuildConfig()

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> { return Promise.resolve(); }
        public testFindStronglyConnectedComponents(fileMap: DependencyFileMap): Map<string, Set<string>> {
            return (this as any).findStronglyConnectedComponents(fileMap);
        }
        protected createHash(input: string): string { return 'cycle-group-' + input.length; }
    }

    const baseMode = new TestBaseMode(mockConfig as any);
    const graph1 = {
        dependencies: { 'A': ['B', 'C'], 'B': ['C'], 'C': ['A'] },
        dependants: { 'A': ['C'], 'B': ['A'], 'C': ['A', 'B'] }
    };
    const result1 = baseMode.testFindStronglyConnectedComponents(graph1);
    expect(result1.size).toBe(1);
    expect(Array.from(result1.values())[0].size).toBe(3);
    const graph2 = {
        dependencies: { 'A': ['B', 'C'], 'B': ['D'], 'C': ['D'], 'D': ['E'], 'E': ['B'] },
        dependants: { 'A': [], 'B': ['A', 'E'], 'C': ['A'], 'D': ['B', 'C'], 'E': ['D'] }
    };
    const result2 = baseMode.testFindStronglyConnectedComponents(graph2);
    expect(result2.size).toBe(1);
    expect(Array.from(result2.values())[0].size).toBe(3);
    const graph3 = {
        dependencies: { 'A': ['B'], 'B': ['C'], 'C': ['D'], 'D': [], 'E': ['F'], 'F': ['E'] },
        dependants: { 'A': [], 'B': ['A'], 'C': ['B'], 'D': ['C'], 'E': ['F'], 'F': ['E'] }
    };
    const result3 = baseMode.testFindStronglyConnectedComponents(graph3);
    expect(result3.size).toBe(1);
    expect(Array.from(result3.values())[0].size).toBe(2);
    const graph4 = {
        dependencies: { 'A': ['B'], 'B': ['C'], 'C': ['D'], 'D': [] },
        dependants: { 'A': [], 'B': ['A'], 'C': ['B'], 'D': ['C'] }
    };
    const result4 = baseMode.testFindStronglyConnectedComponents(graph4);
    expect(result4.size).toBe(0);
}

function test_createExternalProgramJob_branches() {
    const mockConfig: BuildConfig = mock.getMockedBuildConfig()

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testCreateExternalProgramJob(id: string, fileList: string[],
            jobs: Record<string, JobInfo>, dependencies: Set<string>, isInCycle?: boolean): void {
            return (this as any).createExternalProgramJob(id, fileList, jobs, dependencies, isInCycle);
        }
    }

    const baseMode = new TestBaseMode(mockConfig);

    {
        const id = "external-program:test/file.ets";
        const fileList = ["test/file.ets"];
        const jobs: Record<string, JobInfo> = {};
        const dependencies = new Set<string>([id, "external-program:other.ets"]);
        const isInCycle = false;

        baseMode.testCreateExternalProgramJob(id, fileList, jobs, dependencies, isInCycle);

        expect(dependencies.has(id)).toBe(false);
        expect(dependencies.size).toBe(1);

        expect(jobs[id]).toBeDefined();
        expect(jobs[id].id).toBe(id);
        expect(jobs[id].fileList).toEqual(fileList);
        expect(jobs[id].jobDependencies).toEqual(["external-program:other.ets"]);
        expect(jobs[id].jobDependants).toEqual([]);
    }

    {
        const id = "external-program:test/file2.ets";
        const fileList = ["test/file2.ets", "test/file2b.ets"];
        const jobs: Record<string, JobInfo> = {
            [id]: {
                id,
                fileList: ["test/file2.ets"],
                isAbcJob: false,
                jobDependencies: ["external-program:dep1.ets"],
                jobDependants: ["external-program:dep3.ets"]
            }
        };

        const dependencies = new Set<string>(["external-program:dep2.ets"]);
        const isInCycle = true;

        baseMode.testCreateExternalProgramJob(id, fileList, jobs, dependencies, isInCycle);

        expect(jobs[id]).toBeDefined();
        expect(jobs[id].id).toBe(id);
        expect(jobs[id].fileList).toEqual(["test/file2.ets"]);
        expect(jobs[id].jobDependencies).toContain("external-program:dep1.ets");
        expect(jobs[id].jobDependencies).toContain("external-program:dep2.ets");
        expect(jobs[id].jobDependencies.length).toBe(2);
        expect(jobs[id].jobDependants).toEqual(["external-program:dep3.ets"]);
    }
}

// NOTE: to be defined later
/*
function test_collectCompileFiles_bytecode_har() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockConfig: BuildConfig = getMockConfig()

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testProcessEntryFiles(): void {
            super.processEntryFiles();
        }

        // NOTE: to be defined later
        // public testCollectAbcFileFromByteCodeHar(): void {
        //     this.collectAbcFileFromByteCodeHar();
        // }
    }

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);
    const baseMode = new TestBaseMode(mockConfig);


    baseMode.moduleInfos = new Map();
    baseMode.moduleInfos.set("test", {
        packageName: "test",
        moduleType: "har",
        byteCodeHar: true,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "./dist/cache/test/config.json",
        compileFileInfos: []
    });

    global.getFileHash = jest.fn().mockReturnValue("hash123");

    jest.spyOn(baseMode, 'testCollectAbcFileFromByteCodeHar').mockImplementation(() => { });

    baseMode.testCollectCompileFiles();
}
*/

function test_processEntryFiles_file_not_in_module() {
    const mockConfig: BuildConfig = mock.getMockedBuildConfig()

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testProcessEntryFiles(): void {
            super.processEntryFiles();
        }
    }

    const baseMode = new TestBaseMode(mockConfig);

    baseMode.entryFiles = new Set([
        '/other/path/test.ets'
    ]);

    baseMode.moduleInfos = new Map();
    baseMode.moduleInfos.set("test", getMockMainModuleInfo())

    baseMode.testProcessEntryFiles();

    expect(Logger.getInstance().printError).toHaveBeenCalledWith(
        expect.objectContaining({
            code: ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
            description: 'File does not belong to any module in moduleInfos.'
        })
    );

    expect(baseMode.fileToModule.size).toBe(0);
}

// NOTE: to be defined later
/*
function test_processEntryFiles_decl_ets_skip() {
    const mockConfig: BuildConfig = getMockConfig()

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testProcessEntryFiles(): void {
            this.processEntryFiles();
        }
    }

    const baseMode = new TestBaseMode(mockConfig);

    baseMode.cacheDir = "./dist/cache";
    baseMode.abcFiles = new Set();
    baseMode.filesHashCache = {};

    baseMode.entryFiles = new Set([
        'index.ets',
        '/test/ut/mock/web.d.ets'
    ]);

    baseMode.moduleInfos = new Map();
    baseMode.moduleInfos.set("test", {
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "./dist/cache/test/config.json",
        compileFileInfos: []
    });

    global.getFileHash = jest.fn().mockReturnValue("hash123");

    baseMode.testProcessEntryFiles();
}
*/

function test_collectModuleInfos() {
    const mockConfig: BuildConfig = {
        buildMode: BUILD_MODE.DEBUG,
        compileFiles: ["test.ets"],
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        plugins: {},
        buildType: BUILD_TYPE.BUILD,
        hasMainModule: true,
        moduleType: OHOS_MODULE_TYPE.HAR,
        arkts: {} as any,
        arktsGlobal: {} as any,
        enableDeclgenEts2Ts: false,
        byteCodeHar: false,
        declgenV1OutPath: "./dist/declgen",
        declgenV2OutPath: "./dist/declgen/v2",
        buildSdkPath: "./sdk",
        externalApiPaths: [],

        dependencyModuleList: [
            {
                "packageName": "harA",
                "moduleName": "harA",
                "moduleType": "har",
                "modulePath": "test/ut/mock/demo_1.2_dep_hsp1.2/harA",
                "sourceRoots": ["./"],
                "entryFile": "index.ets",
                "language": "11.2",
                "dependencies": ["hspA"],
                "byteCodeHar": false
            },
            {
                "packageName": "hspA",
                "moduleName": "hspA",
                "moduleType": "shared",
                "modulePath": "hspA",
                "sourceRoots": ["./"],
                "entryFile": "index.ets",
                "language": "11.2",
                "byteCodeHar": false
            }
        ]
    } as any;
    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance(mockConfig);
    let baseModule: BuildMode = new BuildMode(mockConfig);
    (baseModule as any).collectModuleInfos();

    expect(Logger.getInstance().printError).not.toHaveBeenCalledWith(
        expect.objectContaining({
            code: ErrorCode.BUILDSYSTEM_MODULE_INFO_NOT_CORRECT_FAIL,
            description: 'Main module info from hvigor is not correct.'
        })
    );
}

function test_collectDependentCompileFiles002() {
    const mockLogger = {
        printError: jest.fn(),
        printInfo: jest.fn(),
        hasErrors: jest.fn().mockReturnValue(false)
    };

    const moduleRootPath = "test/ut/mock/";
    const testFile = `${moduleRootPath}a.ets`;

    const mockConfig: BuildConfig = {
        compileFiles: [testFile],
        packageName: "entry",
        moduleType: OHOS_MODULE_TYPE.HAR,
        buildType: BUILD_TYPE.BUILD,
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: moduleRootPath,
        sourceRoots: ["./"],
        loaderOutPath: "test/ut/mock/dist",
        cachePath: "test/ut/mock/dist/cache",
        dependencyModuleList: [],
        plugins: {},
        hasMainModule: false,
        arkts: {} as any,
        arktsGlobal: {} as any,
        enableDeclgenEts2Ts: false,
        byteCodeHar: false,
        declgenV1OutPath: "./dist/declgen",
        declgenV2OutPath: "./dist/declgen/v2",
        buildSdkPath: "./sdk",
        externalApiPaths: []
    } as any;

    const BuildMode = require('../../../src/build/build_mode').BuildMode;
    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance(mockConfig);
    let baseModule = new BuildMode(mockConfig);

    (baseModule as any).logger = mockLogger;
    (baseModule as any).moduleInfos = new Map();
    (baseModule as any).moduleInfos.set("entry", {
        packageName: "entry",
        moduleRootPath: moduleRootPath,
        sourceRoots: ["./"],
        compileFileInfos: []
    });

    (baseModule as any).entryFiles = new Set([testFile]);
    (baseModule as any).dependencyFileMap = {
        dependants: {
            [testFile]: ["dependency1.ets", "dependency2.ets"]
        }
    };
    (baseModule as any).cacheDir = "test/ut/mock/dist/cache";
    (baseModule as any).hashCache = {};
    (baseModule as any).abcFiles = new Set();
    (baseModule as any).compileFiles = new Map();

    (baseModule as any).isBuildConfigModified = true;

    (baseModule as any).isFileChanged = jest.fn().mockReturnValue(false);

    (baseModule as any).collectDependentCompileFiles();

    expect(mockLogger.printError).not.toHaveBeenCalledWith(
        expect.objectContaining({
            code: ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
            message: 'File does not belong to any module in moduleInfos.'
        })
    );

    expect((baseModule as any).abcFiles.size).toBe(1);
    const compileFilesArray = Array.from((baseModule as any).compileFiles.keys());
    expect(compileFilesArray.length).toBe(1);
    expect(compileFilesArray[0]).toBe(testFile);
}

// NOTE: to be defined later
/*
function test_shouldSkipFile() {
    const mockConfig: BuildConfig = getMockBuildConfig()
    let baseModule: BuildMode = new BuildMode(mockConfig);
    baseModule.filesHashCache = {
        "/test/path/file.ets": "hash123"
    };

    const file = "/test/path/file.ets";
    const moduleInfo: ModuleInfo = {
        isMainModule: false,
        packageName: "test",
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        arktsConfigFile: "/cache/test/arktsconfig.json",
        declgenV1OutPath: "/dist/declgen",
        declgenBridgeCodePath: "/dist/bridge",
        dynamicDependencyModules: new Map(),
        staticDependencyModules: new Map(),
        dependencies: [],
        moduleType: OHOS_MODULE_TYPE.HAR,
        entryFile: "index.ets",
        declgenV2OutPath: "/dist/declgen/v2",
        byteCodeHar: false
    };
    const filePathFromModuleRoot = "file.ets";
    const abcFilePath = "/cache/test/file.abc";

    baseModule.enableDeclgenEts2Ts = true;
    let result3 = baseModule.shouldSkipFile(file, moduleInfo, filePathFromModuleRoot, abcFilePath);
    baseModule.enableDeclgenEts2Ts = false;
    let result4 = baseModule.shouldSkipFile(file, moduleInfo, filePathFromModuleRoot, abcFilePath);
    expect(result3).toBe(false);
    expect(result4).toBe(false);
}

function test_assignTaskToIdleWorker_empty_queues() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockConfig = {
        packageName: "test",
        moduleType: "har",
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
    };

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }

        public testAssignTaskToIdleWorker(
            workerInfo: WorkerInfo,
            queues: Queues,
            processingJobs: Set<string>,
            serializableConfig: Object,
            globalContextPtr: any
        ): void {
            (this as any).assignTaskToIdleWorker(
                workerInfo,
                queues,
                processingJobs,
                serializableConfig,
                globalContextPtr
            );
        }
    }
    const baseMode = new TestBaseMode(mockConfig as any);
    const mockWorker = {
        postMessage: jest.fn()
    };

    const workerInfo: WorkerInfo = {
        worker: mockWorker as unknown as ThreadWorker,
        isIdle: true
    };

    const queues: Queues = {
        externalProgramQueue: [],
        abcQueue: []
    };

    const processingJobs = new Set<string>();
    const serializableConfig = {};
    const globalContextPtr = {};

    (baseMode as any).allFiles = new Map([
        ['test/file.ets', {
            filePath: 'test/file.ets',
            packageName: 'test',
            arktsConfigFile: 'test/config.json',
            abcFilePath: './dist/file.abc'
        }]
    ]);

    const postMessageSpy = jest.spyOn(mockWorker, 'postMessage');

    baseMode.testAssignTaskToIdleWorker(
        workerInfo,
        queues,
        processingJobs,
        serializableConfig,
        globalContextPtr
    );

    expect(postMessageSpy).not.toHaveBeenCalled();
    expect(processingJobs.size).toBe(0);
    expect(workerInfo.isIdle).toBe(true);
    jest.restoreAllMocks();
}

function test_assignTaskToIdleWorker_abcQueue_no_job() {
    const mockLogger = {
        printInfo: jest.fn(),
        printError: jest.fn()
    };

    const mockConfig = {
        packageName: "test",
        moduleType: "har",
        buildMode: BUILD_MODE.DEBUG,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        dependentModuleList: [],
    };

    const Logger = require('../../../src/logger').Logger;
    Logger.instance = null;
    Logger.getInstance = jest.fn().mockReturnValue(mockLogger);

    class TestBaseMode extends BaseMode {
        public run(): Promise<void> {
            return Promise.resolve();
        }
        public testAssignTaskToIdleWorker(
            workerInfo: WorkerInfo,
            queues: Queues,
            processingJobs: Set<string>,
            serializableConfig: Object,
            globalContextPtr: any
        ): void {
            (this as any).assignTaskToIdleWorker(
                workerInfo,
                queues,
                processingJobs,
                serializableConfig,
                globalContextPtr
            );
        }
    }

    const baseMode = new TestBaseMode(mockConfig as any);

    const mockWorker = {
        postMessage: jest.fn()
    };

    const workerInfo: WorkerInfo = {
        worker: mockWorker as unknown as ThreadWorker,
        isIdle: true
    };

    const queues: Queues = {
        externalProgramQueue: [],
        abcQueue: [{
            id: 'abc:test/nonexistentfile.ets',
            type: 'abc',
            dependencies: [],
            dependants: [],
            result: null,
            fileList: ['test/nonexistentfile.ets'],
            isDeclFile: false,
            isAbcJob: true
        }]
    };

    const processingJobs = new Set<string>();
    const serializableConfig = {};
    const globalContextPtr = {};

    (baseMode as any).allFiles = new Map([
        ['test/otherfile.ets', {
            filePath: 'test/otherfile.ets',
            packageName: 'test',
            arktsConfigFile: 'test/config.json',
            abcFilePath: './dist/otherfile.abc'
        }]
    ]);

    const consoleSpy = jest.spyOn(console, 'error').mockImplementation(() => { });
    const postMessageSpy = jest.spyOn(mockWorker, 'postMessage');
    try {
        baseMode.testAssignTaskToIdleWorker(
            workerInfo,
            queues,
            processingJobs,
            serializableConfig,
            globalContextPtr
        );
        fail('Expected method to throw, but it did not');
    } catch (error) {
        expect(error).toBeInstanceOf(ReferenceError);
        expect(workerInfo.isIdle).toBe(false);
    } finally {
        consoleSpy.mockRestore();
        jest.restoreAllMocks();
    }
}
*/
