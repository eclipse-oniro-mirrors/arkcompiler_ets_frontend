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
import * as os from 'os';
import { ErrorCode, DriverError, DriverErrorList } from '../../../src/util/error';
import { JobContentType, CompileJobType } from '../../../src/types';
import { MAX_PATH_LENGTH } from '../../../src/pre_define';

jest.mock('os', () => ({
    type: jest.fn(() => 'Windows_NT')
}));

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
        arkts: {
            proceedToState: jest.fn(),
            EtsScript: { fromContext: jest.fn() },
            generateTsDeclarationsFromContext: jest.fn(),
            generateStaticDeclarationsFromContext: jest.fn(),
            formOutputPathForFile: jest.fn((p: string) => {
                if (p.includes('__LONG_OUTPUT_PATH__')) {
                    return 'a'.repeat(300) + '.abc';
                }
                return p.replace('.ets', '.abc');
            }),
            Config: { create: jest.fn(() => ({ peer: {} })) },
            destroyConfig: jest.fn(),
            Context: { createContextSimultaneousMode: jest.fn(() => ({ peer: {}, program: {} })) },
            Es2pandaContextState: { ES2PANDA_STATE_PARSED: 'PARSED', ES2PANDA_STATE_CHECKED: 'CHECKED', ES2PANDA_STATE_ASM_GENERATED: 'ASM', ES2PANDA_STATE_BIN_GENERATED: 'BIN' }
        },
        arktsGlobal: {
            filePath: '',
            config: {},
            compilerContext: { peer: {}, program: {} },
            es2panda: { _DestroyContext: jest.fn(), _FreeCompilerPartMemory: jest.fn() }
        }
    }))
}));
jest.mock('../../../src/plugins/plugins_driver', () => ({
    PluginDriver: {
        getInstance: jest.fn(() => ({
            initPlugins: jest.fn(),
            getPluginContext: jest.fn(() => ({
                setArkTSProgram: jest.fn(),
                setArkTSAst: jest.fn()
            })),
            runPluginHook: jest.fn()
        }))
    },
    PluginHook: { PARSED: 'PARSED', CHECKED: 'CHECKED', CLEAN: 'CLEAN' }
}));
jest.mock('../../../src/util/utils', () => {
    const actualUtils = jest.requireActual('../../../src/util/utils');
    return {
        ...actualUtils,
        ensurePathExists: jest.fn(),
        changeDeclgenFileExtension: (p: string) => p.replace(/\.ets$/, '.d.ets'),
        changeFileExtension: jest.fn((p: string, oldExt: string, newExt: string) => p.replace(oldExt, newExt)),
        createFileIfNotExists: jest.fn(),
        toUnixPath: jest.fn((p: string) => p)
    };
});
jest.mock('../../../src/util/statsRecorder', () => ({
    StatisticsRecorder: jest.fn(() => ({
        record: jest.fn(),
        writeSumSingle: jest.fn()
    })),
    RecordEvent: { END: 'END' },
    BS_PERF_FILE_NAME: 'bs_record_perf.csv'
}));

function createMockBuildConfig(): any {
    return {
        pandaSdkPath: '/mock/panda/sdk',
        buildSdkPath: '/mock/build/sdk',
        cachePath: '/mock/cache',
        projectRootPath: '/mock/project',
        loaderOutPath: '/mock/output',
        declgenV2OutPath: '/mock/declgen',
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
        buildMode: 'release',
        recordType: 'OFF'
    };
}

function createDeclgenV1JobInfo(outputPath: string, bridgeCodePath: string, inputPath?: string): any {
    const input = inputPath ?? '/short/input.ets';
    return {
        contentType: JobContentType.FILE,
        content: { input, output: '/short/output.abc' },
        moduleName: 'testModule',
        moduleRoot: '/short/root',
        arktsConfig: '/short/arktsconfig.json',
        fileToModuleMap: {
            [input]: {
                isMainModule: true,
                packageName: 'testPackage',
                moduleRootPath: '/short/root',
                moduleType: 'ets',
                sourceRoots: ['src'],
                entryFile: 'index.ets',
                arktsConfigFile: '/short/arktsconfig.json',
                declgenV1OutPath: outputPath,
                declgenBridgeCodePath: bridgeCodePath,
                dependencies: [],
                staticDependencyModules: new Map(),
                dynamicDependencyModules: new Map()
            }
        }
    };
}

function createCompileJobInfo(inputPath: string, jobType?: number): any {
    return {
        contentType: JobContentType.FILE,
        content: { input: inputPath, output: '/short/output.abc' },
        moduleName: 'testModule',
        moduleRoot: '/short/root',
        arktsConfig: '/short/arktsconfig.json',
        jobType: jobType ?? CompileJobType.ABC
    };
}

describe('Ets2panda path length validation', () => {
    let Ets2panda: any;

    beforeEach(() => {
        jest.clearAllMocks();

        (fs.existsSync as jest.Mock).mockReturnValue(true);
        (fs.readFileSync as jest.Mock).mockReturnValue('let x = 1;');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.mkdirSync as jest.Mock).mockReturnValue(undefined);
        (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: Date.now() });

        Ets2panda = require('../../../src/util/ets2panda').Ets2panda;
        Ets2panda.destroyInstance();
    });

    afterEach(() => {
        Ets2panda.destroyInstance();
    });

    describe('declgenV1', () => {
        test('should throw DriverError when declEtsOutputPath exceeds MAX_PATH_LENGTH', () => {
            const longOutputPath = 'a'.repeat(MAX_PATH_LENGTH + 50);
            const jobInfo = createDeclgenV1JobInfo(longOutputPath, '/short/bridge');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            expect(() => ets2panda.declgenV1(jobInfo, true, true)).toThrow();
            try {
                ets2panda.declgenV1(jobInfo, true, true);
            } catch (e: any) {
                expect(e.logData?.code).toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
            }
        });

        test('should throw DriverError when etsOutputPath exceeds MAX_PATH_LENGTH', () => {
            const longBridgeCodePath = 'b'.repeat(MAX_PATH_LENGTH + 50);
            const jobInfo = createDeclgenV1JobInfo('/short/output', longBridgeCodePath);

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            expect(() => ets2panda.declgenV1(jobInfo, true, true)).toThrow();
            try {
                ets2panda.declgenV1(jobInfo, true, true);
            } catch (e: any) {
                expect(e.logData?.code).toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
            }
        });

        test('should throw DriverError when staticRecordPath exceeds MAX_PATH_LENGTH', () => {
            const jobInfo = createDeclgenV1JobInfo('c'.repeat(MAX_PATH_LENGTH + 50), '/short/bridge');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            expect(() => ets2panda.declgenV1(jobInfo, true, true)).toThrow();
            try {
                ets2panda.declgenV1(jobInfo, true, true);
            } catch (e: any) {
                expect(e.logData?.code).toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
            }
        });

        test('should not throw BUILDSYSTEM_PATH_TOO_LONG when output paths are within MAX_PATH_LENGTH', () => {
            const jobInfo = createDeclgenV1JobInfo('/short/output', '/short/bridge');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            try {
                ets2panda.declgenV1(jobInfo, true, true);
            } catch (e: any) {
                expect(e.logData?.code).not.toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
            }
        });
    });

    describe('compile', () => {
        test('should use code description and message from two-line diagnostics', () => {
            const jobInfo = createCompileJobInfo('/short/input.ets');
            const diagnosticMessage = [
                'Failed to proceed to ES2PANDA_STATE_CHECKED',
                '',
                '1 ERROR: 11503319 Semantic error',
                'Error Message: Type \'Int\' is not compatible with type \'String\' at property \'age\'',
                '2 ERROR: 11503318 Semantic error',
                'Error Message: Type \'Int\' cannot be assigned to type \'String\''
            ].join('\n');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());
            (ets2panda as any).koalaModule.arkts.proceedToState.mockImplementationOnce(() => {
                throw new Error(diagnosticMessage);
            });

            let errorList: DriverErrorList | undefined;
            try {
                ets2panda.compile('testJob', jobInfo);
            } catch (e: any) {
                errorList = e;
            }

            expect(errorList).toBeInstanceOf(DriverErrorList);
            const errors = errorList!.errors;
            expect(errors).toHaveLength(2);
            expect(errors[0]).toBeInstanceOf(DriverError);
            expect(errors[0].logData.code).toBe('11503319');
            expect(errors[0].logData.description).toBe('Semantic error');
            expect(errors[0].logData.cause).toBe(
                'Type \'Int\' is not compatible with type \'String\' at property \'age\''
            );
            expect(errors[1]).toBeInstanceOf(DriverError);
            expect(errors[1].logData.code).toBe('11503318');
            expect(errors[1].logData.description).toBe('Semantic error');
            expect(errors[1].logData.cause).toBe(
                'Type \'Int\' cannot be assigned to type \'String\''
            );
        });

        test('should throw DriverError with BUILDSYSTEM_PATH_TOO_LONG when output path exceeds MAX_PATH_LENGTH', () => {
            const jobInfo = createCompileJobInfo('/short/__LONG_OUTPUT_PATH__.ets');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            expect(() => ets2panda.compile('testJob', jobInfo)).toThrow();
            try {
                ets2panda.compile('testJob', jobInfo);
            } catch (e: any) {
                expect(e.logData?.code).toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
                expect(e.logData?.code).not.toBe(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL);
            }
        });

        test('should not throw BUILDSYSTEM_PATH_TOO_LONG when output path is within MAX_PATH_LENGTH', () => {
            const jobInfo = createCompileJobInfo('/short/input.ets');

            const ets2panda = Ets2panda.getInstance(createMockBuildConfig());

            try {
                ets2panda.compile('testJob', jobInfo);
            } catch (e: any) {
                expect(e.logData?.code).not.toBe(ErrorCode.BUILDSYSTEM_PATH_TOO_LONG);
            }
        });
    });
});
