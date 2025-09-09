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

import { ArkTSConfigGenerator, ArkTSConfig } from '../../../src/build/generate_arktsconfig';
import { BuildConfig, BUILD_MODE, BUILD_TYPE, ModuleInfo, OHOS_MODULE_TYPE } from '../../../src/types';
import { mockLogger, moduleInfoWithNullSourceRoots, moduleInfoWithFalseEts2Ts } from '../mock/mockData';
import * as fs from 'fs';

// Define the AliasConfig interface for testing
interface AliasConfig {
    isStatic: boolean;
    originalAPIName: string;
    [key: string]: any;
}

// generate_arktsconfig test suite.
describe('test generate_arktsconfig.ts file api', () => {

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
    });

    test('should throw error if buildConfig or moduleInfos is not provided on first instantiation', () => {
        expect(() => {
            ArkTSConfigGenerator.getInstance();
        }).toThrow('buildConfig and moduleInfos is required for the first instantiation of ArkTSConfigGenerator.');
    });

});

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
    dependentModuleList: [
    ]
} as any;

describe('test writeArkTSConfigFile in normal and abnormal scenarios', () => {
    let generator: ArkTSConfigGenerator;
    beforeEach(() => {
        // Mock ArkTSConfigGenerator instance
        generator = Object.create(ArkTSConfigGenerator.prototype);
        (generator as any).getPathSection = jest.fn().mockReturnValue({
            std: ['/path/to/stdlib/std'],
            escompat: ['/path/to/stdlib/escompat'],
        });
        (generator as any).getDependenciesSection = jest.fn();
        (generator as any).getDynamicPathSection = jest.fn();
        (generator as any).logger = mockLogger;
        (generator as any).dynamicSDKPaths = ['/sdk/apis/interop'];
        (generator as any).arktsconfigs = new Map();
    });

    afterEach(() => {
        jest.clearAllMocks();
    });

    test('should throw error if sourceRoots is empty', () => {
        expect(() => {
            generator.generateArkTSConfigFile(moduleInfoWithNullSourceRoots, false);
        }).toThrow('Exit with error.');
    });

    test('should generate correct arktsConfig when enableDeclgenEts2Ts is false', () => {
        generator.generateArkTSConfigFile(moduleInfoWithFalseEts2Ts, false);
        expect((generator as any).getDependenciesSection).toHaveBeenCalled();
    })
});

describe('test if the generateSystemSdkPathSection is working correctly', () => {
    test('should traverse directories and add correct paths to pathSection', () => {
        const fs = require('fs');
        const path = require('path');

        jest.spyOn(fs, 'existsSync').mockImplementation(function(path) {
            return ['/sdk/api', '/sdk/arkts'].includes(path as string);
        });

        jest.spyOn(fs, 'readdirSync').mockImplementation(function(dir) {
            if (dir === '/sdk/api') {
                return ['web.d.ets', 'component', 'arkui'];
            }
            if (dir === '/sdk/api/component') {
                return ['button.d.ets', 'text.d.ets'];
            }
            if (dir === '/sdk/api/arkui') {
                return ['runtime-api'];
            }
            if (dir === '/sdk/api/arkui/runtime-api') {
                return ['special.d.ets'];
            }
            if (dir === '/sdk/arkts') {
                return ['common.d.ets', 'utils'];
            }
            if (dir === '/sdk/arkts/utils') {
                return ['helper.d.ets'];
            }
            return [];
        });

        jest.spyOn(fs, 'statSync').mockImplementation(function(itemPath) {
            const isFile = (itemPath as string).endsWith('.d.ets');
            return {
                isFile: () => isFile,
                isDirectory: () => !isFile
            } as fs.Stats;
        });

        jest.spyOn(path, 'basename').mockImplementation(function(p, ext) {
            const base = (p as string).split('/').pop() || '';
            return ext && base.endsWith(ext as string) ? base.slice(0, -(ext as string).length) : base;
        });

        jest.spyOn(path, 'join').mockImplementation(function() {
            return Array.from(arguments).join('/');
        });

        jest.spyOn(path, 'resolve').mockImplementation(function(a, b) {
            return `${a as string}/${b as string}`;
        });

        (global as any).changeFileExtension = jest.fn(function(filePath, newExt, oldExt) {
            return (filePath as string).replace(oldExt as string, newExt as string);
        });

        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.systemSdkPath = '/sdk';
        generator.logger = {
            printError: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn()
        };

        const pathSection: Record<string, string[]> = {};
        (ArkTSConfigGenerator.prototype as any).generateSystemSdkPathSection.call(generator, pathSection);

        expect(generator.logger.printWarn).toHaveBeenCalledWith('sdk path /sdk/kits not exist.');

        delete (global as any).changeFileExtension;
    });

    test('should use externalApiPaths when provided', () => {
        const fs = require('fs');
        const path = require('path');

        jest.spyOn(fs, 'existsSync').mockImplementation(function(path) {
            return path === '/external/api';
        });

        jest.spyOn(fs, 'readdirSync').mockImplementation(function(dir) {
            if (dir === '/external/api') {
                return ['external.d.ets', 'widgets'];
            }
            if (dir === '/external/api/widgets') {
                return ['widget.d.ets'];
            }
            return [];
        });

        jest.spyOn(fs, 'statSync').mockImplementation(function(itemPath) {
            const isFile = (itemPath as string).endsWith('.d.ets');
            return {
                isFile: () => isFile,
                isDirectory: () => !isFile
            } as fs.Stats;
        });

        jest.spyOn(path, 'basename').mockImplementation(function(p, ext) {
            const base = (p as string).split('/').pop() || '';
            return ext && base.endsWith(ext as string) ? base.slice(0, -(ext as string).length) : base;
        });

        jest.spyOn(path, 'join').mockImplementation(function() {
            return Array.from(arguments).join('/');
        });

        jest.spyOn(path, 'resolve').mockImplementation(function(a, b) {
            return `${a as string}/${b as string}`;
        });

        (global as any).changeFileExtension = jest.fn(function(filePath, newExt, oldExt) {
            return (filePath as string).replace(oldExt as string, newExt as string);
        });

        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.systemSdkPath = '/sdk';
        generator.externalApiPaths = ['/external/api', '/nonexistent/path'];
        generator.logger = {
            printError: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn()
        };

        const pathSection: Record<string, string[]> = {};
        (ArkTSConfigGenerator.prototype as any).generateSystemSdkPathSection.call(generator, pathSection);

        expect(pathSection).toEqual({
            'escompat': [undefined],
            'external': ['/external/api/external'],
            'std': [undefined],
            'widgets.widget': ['/external/api/widgets/widget']
        });

        expect(generator.logger.printWarn).toHaveBeenCalledWith('sdk path /nonexistent/path not exist.');

        delete (global as any).changeFileExtension;
    });

    test('should skip non-allowed file extensions', () => {
        const fs = require('fs');
        const path = require('path');

        jest.spyOn(fs, 'existsSync').mockReturnValue(true);

        jest.spyOn(fs, 'readdirSync').mockImplementation(function(dir) {
            if (dir === '/sdk/api') {
                return [
                    'valid.d.ets',
                    'invalid.js',
                    'another.ts',
                    'weird.d.ets.bak'
                ];
            }
            return [];
        });

        jest.spyOn(fs, 'statSync').mockImplementation(function(itemPath) {
            return {
                isFile: () => true,
                isDirectory: () => false
            } as fs.Stats;
        });

        jest.spyOn(path, 'basename').mockImplementation(function(p, ext) {
            const base = (p as string).split('/').pop() || '';
            return ext && base.endsWith(ext as string) ? base.slice(0, -(ext as string).length) : base;
        });

        jest.spyOn(path, 'join').mockImplementation(function() {
            return Array.from(arguments).join('/');
        });

        jest.spyOn(path, 'resolve').mockImplementation(function(a, b) {
            return `${a}/${b}`;
        });

        (global as any).changeFileExtension = jest.fn(function(filePath, newExt, oldExt) {
            return filePath.replace(oldExt, newExt);
        });

        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.systemSdkPath = '/sdk';
        generator.logger = {
            printError: jest.fn(),
            printInfo: jest.fn(),
            printWarn: jest.fn()
        };

        const pathSection: Record<string, string[]> = {};
        (ArkTSConfigGenerator.prototype as any).generateSystemSdkPathSection.call(generator, pathSection);

        delete (global as any).changeFileExtension;
    });
});

describe('test if the getDependenciesSection is working correctly', () => {
    test('should properly process dynamic dependencies and their declaration files', () => {
        const fs = require('fs');
        const path = require('path');

        jest.spyOn(fs, 'existsSync').mockReturnValue(true);
        jest.spyOn(fs, 'readFileSync').mockReturnValue(JSON.stringify({
            files: {
                'src/file1.ets': {
                    declPath: '/modules/dep1/dist/file1.d.ts',
                    ohmUrl: 'dep1/file1'
                },
                'src/index.ets': {
                    declPath: '/modules/dep1/dist/index.d.ts',
                    ohmUrl: 'dep1/index'
                },
                'src/file2.ets': {
                    declPath: '/modules/dep1/dist/file2.d.ts',
                    ohmUrl: 'dep1/file2'
                }
            }
        }));

        jest.spyOn(path, 'resolve').mockImplementation((root, file) => `${root}/${file}`);

        (global as any).changeFileExtension = jest.fn((filePath, newExt) => {
            return filePath.replace(/\.[^/.]+$/, newExt);
        });

        const generator = Object.create(ArkTSConfigGenerator.prototype);
        generator.systemDependenciesSection = {};

        generator.getOhmurl = jest.fn((file, depModuleInfo) => {
            return `${depModuleInfo.packageName}/${file.replace(/^src\//, '').replace(/\.ets$/, '')}`;
        });

        const depModuleInfo: ModuleInfo = {
            packageName: "dep1",
            moduleRootPath: "/modules/dep1",
            sourceRoots: ["src"],
            entryFile: "/modules/dep1/src/index.ets",
            declFilesPath: "/modules/dep1/dist/decls.json",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map(),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: false,
            byteCodeHar: false
        } as any;

        const moduleInfo: ModuleInfo = {
            packageName: "mainModule",
            moduleRootPath: "/modules/main",
            sourceRoots: ["src"],
            entryFile: "/modules/main/src/index.ets",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map([
                ["dep1", depModuleInfo]
            ]),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: true,
            byteCodeHar: false
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).getDependenciesSection.call(generator, moduleInfo, arktsconfig);

        expect(fs.existsSync).toHaveBeenCalledWith("/modules/dep1/dist/decls.json");
        expect(fs.readFileSync).toHaveBeenCalledWith("/modules/dep1/dist/decls.json", "utf-8");
        expect(path.resolve).toHaveBeenCalledWith("/modules/dep1", "src/file1.ets");
        expect(path.resolve).toHaveBeenCalledWith("/modules/dep1", "src/index.ets");
        expect(path.resolve).toHaveBeenCalledWith("/modules/dep1", "src/file2.ets");

        delete (global as any).changeFileExtension;
    });

    test('should skip dependency if declaration file does not exist', () => {
        const fs = require('fs');
        const path = require('path');

        const consoleErrorMock = jest.spyOn(console, 'error').mockImplementation(() => { });

        jest.spyOn(fs, 'existsSync').mockReturnValue(false);

        const generator = Object.create(ArkTSConfigGenerator.prototype);
        generator.systemDependenciesSection = {};

        const depModuleInfo: ModuleInfo = {
            packageName: "dep1",
            moduleRootPath: "/modules/dep1",
            sourceRoots: ["src"],
            entryFile: "/modules/dep1/src/index.ets",
            declFilesPath: "/modules/dep1/dist/decls.json",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map(),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: false,
            byteCodeHar: false
        } as any;

        const moduleInfo: ModuleInfo = {
            packageName: "mainModule",
            moduleRootPath: "/modules/main",
            sourceRoots: ["src"],
            entryFile: "/modules/main/src/index.ets",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map([
                ["dep1", depModuleInfo]
            ]),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: true,
            byteCodeHar: false
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).getDependenciesSection.call(generator, moduleInfo, arktsconfig);

        expect(consoleErrorMock).toHaveBeenCalled();
        expect(consoleErrorMock.mock.calls[0][0]).toContain("mainModule depends on dynamic module dep1");

        expect(fs.existsSync).toHaveBeenCalledWith("/modules/dep1/dist/decls.json");

        consoleErrorMock.mockRestore();
    });

    test('should handle missing declFilesPath property', () => {
        const fs = require('fs');

        const consoleErrorMock = jest.spyOn(console, 'error').mockImplementation(() => { });

        const generator = Object.create(ArkTSConfigGenerator.prototype);
        generator.systemDependenciesSection = {};

        const depModuleInfo: ModuleInfo = {
            packageName: "dep1",
            moduleRootPath: "/modules/dep1",
            sourceRoots: ["src"],
            entryFile: "/modules/dep1/src/index.ets",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map(),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: false,
            byteCodeHar: false
        } as any;

        const moduleInfo: ModuleInfo = {
            packageName: "mainModule",
            moduleRootPath: "/modules/main",
            sourceRoots: ["src"],
            entryFile: "/modules/main/src/index.ets",
            language: "11.0",
            arktsConfigFile: "/config/arktsconfig.json",
            compileFileInfos: [],
            dynamicDepModuleInfos: new Map([
                ["dep1", depModuleInfo]
            ]),
            staticDepModuleInfos: new Map(),
            moduleType: OHOS_MODULE_TYPE.HAR,
            isMainModule: true,
            byteCodeHar: false
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).getDependenciesSection.call(generator, moduleInfo, arktsconfig);

        expect(consoleErrorMock).toHaveBeenCalled();
        expect(consoleErrorMock.mock.calls[0][0]).toContain("mainModule depends on dynamic module dep1");

        consoleErrorMock.mockRestore();
    });
});

describe('test if the processAlias is working correctly', () => {
    test('should handle both static and dynamic aliases correctly', () => {
        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.dynamicSDKPaths = ['/sdk/apis/interop'];
        generator.processStaticAlias = jest.fn();
        generator.processDynamicAlias = jest.fn();
        generator.logger = {
            printWarn: jest.fn(),
            printError: jest.fn(),
            printInfo: jest.fn()
        };

        const aliasConfig: Record<string, Record<string, AliasConfig>> = {};

        const moduleAliasConfig: Record<string, AliasConfig> = {};
        moduleAliasConfig["static1"] = {
            isStatic: true,
            originalAPIName: "@ohos.test1"
        } as AliasConfig;
        moduleAliasConfig["kit1"] = {
            isStatic: false,
            originalAPIName: "@kit.test2"
        } as AliasConfig;
        moduleAliasConfig["dynamic1"] = {
            isStatic: false,
            originalAPIName: "@ohos.test3"
        } as AliasConfig;
        aliasConfig["testModule"] = moduleAliasConfig;
        generator.aliasConfig = aliasConfig;

        const moduleInfo: ModuleInfo = {
            packageName: "testModule",
            moduleRootPath: "/test/path",
            sourceRoots: ["src"],
            arktsConfigFile: "/test/path/config.json",
            compileFileInfos: []
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).processAlias.call(
            generator,
            arktsconfig
        );

        expect(generator.processStaticAlias).toHaveBeenCalledTimes(1);
        expect(generator.processDynamicAlias).toHaveBeenCalledTimes(1);

        expect(generator.processStaticAlias).toHaveBeenCalledWith(
            "kit1",
            { isStatic: false, originalAPIName: "@kit.test2" },
            arktsconfig
        );

        expect(generator.processDynamicAlias).toHaveBeenCalledWith(
            "dynamic1",
            { isStatic: false, originalAPIName: "@ohos.test3" },
            arktsconfig
        );


    });

    test('should handle undefined aliasConfig gracefully', () => {
        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.dynamicSDKPaths = ['/sdk/apis/interop'];
        generator.processStaticAlias = jest.fn();
        generator.processDynamicAlias = jest.fn();
        generator.logger = {
            printWarn: jest.fn(),
            printError: jest.fn(),
            printInfo: jest.fn()
        };

        const aliasConfig: Record<string, Record<string, AliasConfig>> = {};
        generator.aliasConfig = aliasConfig;

        const moduleInfo: ModuleInfo = {
            packageName: "testModule",
            moduleRootPath: "/test/path",
            sourceRoots: ["src"],
            arktsConfigFile: "/test/path/config.json",
            compileFileInfos: []
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).processAlias.call(
            generator,
            arktsconfig
        );

        expect(generator.processStaticAlias).not.toHaveBeenCalled();
        expect(generator.processDynamicAlias).not.toHaveBeenCalled();
    });

    test('should handle null aliasConfig gracefully', () => {
        const generator = Object.create(ArkTSConfigGenerator.prototype);

        generator.dynamicSDKPaths = ['/sdk/apis/interop'];
        generator.processStaticAlias = jest.fn();
        generator.processDynamicAlias = jest.fn();

        generator.aliasConfig = null;
        generator.logger = {
            printWarn: jest.fn(),
            printError: jest.fn(),
            printInfo: jest.fn()
        };

        const moduleInfo: ModuleInfo = {
            packageName: "testModule",
            moduleRootPath: "/test/path",
            sourceRoots: ["src"],
            arktsConfigFile: "/test/path/config.json",
            compileFileInfos: []
        } as any;

        const arktsconfig: ArkTSConfig = new ArkTSConfig(moduleInfo);

        (ArkTSConfigGenerator.prototype as any).processAlias.call(
            generator,
            arktsconfig
        );

        expect(generator.processStaticAlias).not.toHaveBeenCalled();
        expect(generator.processDynamicAlias).not.toHaveBeenCalled();
    });
});

describe('test if the processDynamicAlias is working correctly', () => {
    test('should handle API with non-existent declaration file', () => {
        const fs = require('fs');
        jest.spyOn(fs, 'existsSync').mockReturnValue(false);

        (global as any).getInteropFilePathByApi = jest.fn().mockReturnValue('/sdk/apis/interop/ohos.missing.d.ts');
        (global as any).getOhmurlByApi = jest.fn().mockReturnValue('@ohos.missing');

        (global as any).LogDataFactory = {
            newInstance: jest.fn().mockReturnValue({
                toString: () => 'Mock Error: Interop SDK File Not Exist',
                code: '11410500',
                message: 'Interop SDK File Not Exist: /sdk/apis/interop/ohos.missing.d.ts'
            })
        };

        (global as any).ErrorCode = {
            BUILDSYSTEM_INTEROP_SDK_NOT_FIND: '11410500'
        };

        const generator = Object.create(ArkTSConfigGenerator.prototype);
        generator.dynamicSDKPaths = ['/sdk/apis/interop'];
        generator.logger = {
            printError: jest.fn(),
            printInfo: jest.fn()
        };

        const aliasName = 'missingAlias';
        const aliasConfig = {
            isStatic: false,
            originalAPIName: '@ohos.missing'
        } as AliasConfig;
        const dynamicPathSection: Record<string, any> = {};

        (ArkTSConfigGenerator.prototype as any).processDynamicAlias.call(
            generator,
            aliasName,
            aliasConfig,
            dynamicPathSection
        );

        expect(fs.existsSync).toHaveBeenCalled();
        delete (global as any).getInteropFilePathByApi;
        delete (global as any).getOhmurlByApi;
        delete (global as any).LogDataFactory;
        delete (global as any).ErrorCode;
    });
});
