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

import { BaseMode } from '../../../src/build/base_mode';
import { BuildConfig, CompileFileInfo, ModuleInfo } from '../../../src/types';
import * as path from 'path';
import * as fs from 'fs';

jest.mock('fs');
jest.mock('../../../src/logger', () => ({
    Logger: {
        getInstance: jest.fn(() => ({
            printDebug: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn(),
            printError: jest.fn()
        }))
    }
}));
jest.mock('../../../src/dependency_analyzer');
jest.mock('../../../src/util/utils', () => ({
    ...jest.requireActual('../../../src/util/utils'),
    ensurePathExists: jest.fn(),
    safeRealpath: jest.fn((p: string) => p),
    changeDeclgenFileExtension: jest.fn((file: string, ext: string) => {
        return file.replace(/\.[^/.]+$/, ext);
    })
}));
jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn(() => ({}))
}));

class TestableBaseMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }

    public testGetOutputFilePaths(fileInfo: CompileFileInfo) {
        return this.getOutputFilePaths(fileInfo);
    }

    public testFileToModule() {
        return this.fileToModule;
    }
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
        ...overrides
    } as BuildConfig;
}

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        packageName: 'testModule',
        moduleRootPath: '/mock/module',
        sourceRoots: ['src'],
        arktsConfigFile: '/mock/arktsconfig.json',
        compileFileInfos: [],
        dynamicDependencyModules: new Map(),
        staticDependencyModules: new Map(),
        declgenV1OutPath: '/mock/declgen/v1',
        declgenBridgeCodePath: '/mock/declgen/bridge',
        ...overrides
    } as ModuleInfo;
}

function createMockCompileFileInfo(overrides: Partial<CompileFileInfo> = {}): CompileFileInfo {
    return {
        inputFilePath: '/mock/module/src/index.ets',
        outputFilePath: '/mock/output/index.abc',
        arktsConfigFile: '/mock/arktsconfig.json',
        ...overrides
    };
}

describe('BaseMode - getOutputFilePaths', () => {
    let testMode: TestableBaseMode;
    const ensurePathExists = require('../../../src/util/utils').ensurePathExists;
    const changeDeclgenFileExtension = require('../../../src/util/utils').changeDeclgenFileExtension;

    beforeEach(() => {
        jest.clearAllMocks();
        
        // Mock fs methods to prevent file system errors
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('{}');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.statSync as jest.Mock).mockReturnValue({ mtimeMs: Date.now() });
        
        const config = createMockBuildConfig();
        testMode = new TestableBaseMode(config);
    });

    describe('Basic Functionality', () => {
        test('should generate correct output paths for file in module root', () => {
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/mock/module',
                declgenV1OutPath: '/mock/declgen/v1',
                declgenBridgeCodePath: '/mock/declgen/bridge'
            });
            const fileInfo = createMockCompileFileInfo({
                inputFilePath: '/mock/module/index.ets'
            });

            testMode.testFileToModule().set('/mock/module/index.ets', moduleInfo);

            const result = testMode.testGetOutputFilePaths(fileInfo);

            expect(result.declEtsOutputPath).toContain('/mock/declgen/v1/myModule/index.d.ets');
            expect(result.glueCodeOutputPath).toContain('/mock/declgen/bridge/myModule/index.ts');
        });

        test('should generate correct output paths for file in subdirectory', () => {
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/mock/module',
                declgenV1OutPath: '/mock/declgen/v1',
                declgenBridgeCodePath: '/mock/declgen/bridge'
            });
            const fileInfo = createMockCompileFileInfo({
                inputFilePath: '/mock/module/src/components/Button.ets'
            });

            testMode.testFileToModule().set('/mock/module/src/components/Button.ets', moduleInfo);

            const result = testMode.testGetOutputFilePaths(fileInfo);

            expect(result.declEtsOutputPath).toContain('/mock/declgen/v1/myModule/src/components/Button.d.ets');
            expect(result.glueCodeOutputPath).toContain('/mock/declgen/bridge/myModule/src/components/Button.ts');
        });

    });
});
