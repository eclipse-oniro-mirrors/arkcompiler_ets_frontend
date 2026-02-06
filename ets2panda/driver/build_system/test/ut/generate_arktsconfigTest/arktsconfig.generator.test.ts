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
import { ArkTSConfig, ArkTSConfigGenerator } from '../../../src/build/generate_arktsconfig';
import { Logger } from '../../../src/logger';
import { BuildConfig, ModuleInfo, AliasConfig } from '../../../src/types';
import { LANGUAGE_VERSION } from '../../../src/pre_define';

/**
 * Mock Settings
 */
jest.mock('fs');
jest.mock('../../../src/logger');
jest.mock('../../../src/util/utils', () => ({
    safeRealpath: jest.fn((p: string) => p),
    ensurePathExists: jest.fn(),
    getInteropFilePathByApi: jest.fn((api: string, sdkPaths?: Set<string>) => {
        if (api.startsWith('@kit')) return `/mock/sdk/${api}.d.ets`;
        if (api.startsWith('@ohos') || api.startsWith('@system')) return `/mock/sdk/api/${api}.d.ets`;
        return '';
    }),
    getOhmurlByApi: jest.fn((api: string) => api.replace('@', '').replace('.d.ets', '')),
    changeFileExtension: jest.fn((file: string, newExt: string = '', oldExt: string = '') => {
        if (oldExt) return file.replace(new RegExp(`${oldExt.replace('.', '\\.')}$`), newExt);
        return file.replace(/\.[^/.]+$/, newExt);
    }),
    isSubPathOf: jest.fn((subPath: string, parentPath: string) => subPath.startsWith(parentPath)),
    toUnixPath: jest.fn((p: string) => p.replace(/\\/g, '/'))
}));

function createMockBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
    return {
        pandaSdkPath: '/mock/panda/sdk',
        buildSdkPath: '/mock/build/sdk',
        cachePath: '/mock/cache',
        projectRootPath: '/mock/project',
        compileFiles: [],
        aliasConfig: {},
        interopSDKPaths: new Set(),
        externalApiPaths: [],
        ...overrides
    } as BuildConfig;
}

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        packageName: 'testModule',
        moduleRootPath: '/test/module',
        sourceRoots: ['src'],
        arktsConfigFile: '/test/arktsconfig.json',
        compileFileInfos: [],
        dynamicDependencyModules: new Map(),
        staticDependencyModules: new Map(),
        entryFile: '/test/module/entry.ts',
        ...overrides
    } as any;
}

function setupBasicMocks() {
    const mockLogger = {
        printDebug: jest.fn(),
        printWarn: jest.fn(),
        printError: jest.fn()
    };
    (Logger.getInstance as jest.Mock).mockReturnValue(mockLogger);
    (fs.existsSync as jest.Mock).mockReturnValue(false);
    (fs.readdirSync as jest.Mock).mockReturnValue([]);
    (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => false, isDirectory: () => false });
    return mockLogger;
}

/**
 * Singleton Pattern and Lifecycle Management
 */
describe('ArkTSConfigGenerator - Singleton Pattern and Lifecycle', () => {
    let mockLogger: any;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Instance Creation', () => {
        test('should require buildConfig for first getInstance call', () => {
            expect(() => ArkTSConfigGenerator.getInstance()).toThrow(
                'buildConfig and moduleInfos is required for the first instantiation'
            );
        });

        test('should create instance with valid buildConfig', () => {
            const config = createMockBuildConfig();
            const instance = ArkTSConfigGenerator.getInstance(config);
            expect(instance).toBeInstanceOf(ArkTSConfigGenerator);
        });

        test('subsequent getInstance calls should return same instance', () => {
            const config = createMockBuildConfig();
            const instance1 = ArkTSConfigGenerator.getInstance(config);
            const instance2 = ArkTSConfigGenerator.getInstance();
            const instance3 = ArkTSConfigGenerator.getInstance();

            expect(instance1).toBe(instance2);
            expect(instance2).toBe(instance3);
        });

        test('subsequent calls should not require buildConfig parameter', () => {
            const config = createMockBuildConfig();
            ArkTSConfigGenerator.getInstance(config);

            expect(() => ArkTSConfigGenerator.getInstance()).not.toThrow();
        });
    });

    describe('Instance Destruction', () => {
        test('instance should be cleared after destroyInstance', () => {
            const config = createMockBuildConfig();
            ArkTSConfigGenerator.getInstance(config);

            ArkTSConfigGenerator.destroyInstance();

            expect(() => ArkTSConfigGenerator.getInstance()).toThrow();
        });

        test('can recreate with new buildConfig after destruction', () => {
            const config1 = createMockBuildConfig({ cachePath: '/cache1' });
            const instance1 = ArkTSConfigGenerator.getInstance(config1);

            ArkTSConfigGenerator.destroyInstance();

            const config2 = createMockBuildConfig({ cachePath: '/cache2' });
            const instance2 = ArkTSConfigGenerator.getInstance(config2);

            expect(instance1).not.toBe(instance2);
            expect(instance2).toBeInstanceOf(ArkTSConfigGenerator);
        });

        test('multiple destroyInstance calls should not throw error', () => {
            const config = createMockBuildConfig();
            ArkTSConfigGenerator.getInstance(config);

            expect(() => {
                ArkTSConfigGenerator.destroyInstance();
                ArkTSConfigGenerator.destroyInstance();
                ArkTSConfigGenerator.destroyInstance();
            }).not.toThrow();
        });
    });

    describe('Public Accessors', () => {
        test('should access aliasConfig', () => {
            const aliasConfig = {
                'pkg': { 'alias': { originalAPIName: '@ohos.test', isStatic: false } }
            };
            const config = createMockBuildConfig({ aliasConfig });
            const generator = ArkTSConfigGenerator.getInstance(config);

            expect(generator.aliasConfig).toBe(aliasConfig);
        });

        test('should access dynamicSDKPaths', () => {
            const sdkPaths = new Set(['/sdk1', '/sdk2']);
            const config = createMockBuildConfig({ interopSDKPaths: sdkPaths });
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            const generator = ArkTSConfigGenerator.getInstance(config);

            expect(generator.dynamicSDKPaths).toBe(sdkPaths);
            expect(generator.dynamicSDKPaths.size).toBe(2);
        });

        test('should access externalApiPaths', () => {
            const apiPaths = ['/api1', '/api2', '/api3'];
            const config = createMockBuildConfig({ externalApiPaths: apiPaths });
            const generator = ArkTSConfigGenerator.getInstance(config);

            expect(generator.externalApiPaths).toEqual(apiPaths);
            expect(generator.externalApiPaths.length).toBe(3);
        });
    });

    describe('Config Caching', () => {
        test('generated config should be cached', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'pkg1' });

            const generated = generator.generateArkTSConfigFile(moduleInfo, false);
            const retrieved = generator.getArktsConfigByPackageName('pkg1');

            expect(retrieved).toBe(generated);
        });

        test('non-existent package name should return undefined', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            expect(generator.getArktsConfigByPackageName('nonexistent')).toBeUndefined();
        });

        test('should support caching multiple package configs', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            const config1 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'pkg1' }),
                false
            );
            const config2 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'pkg2' }),
                false
            );
            const config3 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'pkg3' }),
                false
            );

            expect(generator.getArktsConfigByPackageName('pkg1')).toBe(config1);
            expect(generator.getArktsConfigByPackageName('pkg2')).toBe(config2);
            expect(generator.getArktsConfigByPackageName('pkg3')).toBe(config3);
        });

        test('regenerating same package should update cache', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            const config1 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'pkg' }),
                false
            );
            const config2 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'pkg' }),
                false
            );

            expect(generator.getArktsConfigByPackageName('pkg')).toBe(config2);
            expect(config1).not.toBe(config2);
        });
    });
});

/**
 * Arktsconfig File Generation
 */
describe('ArkTSConfigGenerator - Config File Generation', () => {
    let mockLogger: any;
    let generator: ArkTSConfigGenerator;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
        const config = createMockBuildConfig();
        generator = ArkTSConfigGenerator.getInstance(config);
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Input Validation', () => {
        test('empty sourceRoots should throw error', () => {
            const moduleInfo = createMockModuleInfo({ sourceRoots: [] });
            expect(() => generator.generateArkTSConfigFile(moduleInfo, false))
                .toThrow();
        });

        test('undefined sourceRoots should throw error', () => {
            const moduleInfo = createMockModuleInfo({ sourceRoots: undefined as any });
            expect(() => generator.generateArkTSConfigFile(moduleInfo, false))
                .toThrow();
        });

        test('valid moduleInfo should generate successfully', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result).toBeInstanceOf(ArkTSConfig);
        });
    });

    describe('Basic Config Structure', () => {
        test('should return ArkTSConfig instance', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result).toBeInstanceOf(ArkTSConfig);
            expect(result.object).toBeDefined();
            expect(result.compilerOptions).toBeDefined();
        });

        test('should set correct package name', () => {
            const moduleInfo = createMockModuleInfo({ packageName: 'myPackage' });
            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.packageName).toBe('myPackage');
        });

        test('should include standard library dependencies', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.dependencies['std/core']).toBeDefined();
            expect(result.dependencies['escompat']).toBeDefined();
        });

        test('should call ensurePathExists', () => {
            const ensurePathExists = require('../../../src/util/utils').ensurePathExists;
            const moduleInfo = createMockModuleInfo({ arktsConfigFile: '/custom/path.json' });

            generator.generateArkTSConfigFile(moduleInfo, false);

            expect(ensurePathExists).toHaveBeenCalledWith('/custom/path.json');
        });
    });

    describe('Framework Mode Processing', () => {
        // Destroy parent's instance before each test to allow custom config
        beforeEach(() => {
            ArkTSConfigGenerator.destroyInstance();
        });
        test('should set useEmptyPackage when frameworkMode is true', () => {
            const config = createMockBuildConfig({
                frameworkMode: true,
                useEmptyPackage: true
            });
            const gen = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();
            const result = gen.generateArkTSConfigFile(moduleInfo, false);

            expect(result.object.compilerOptions.useEmptyPackage).toBe(true);
        });

        test('should not set useEmptyPackage when frameworkMode is false', () => {
            const config = createMockBuildConfig({ frameworkMode: false });
            const gen = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = gen.generateArkTSConfigFile(moduleInfo, false);
            expect(result.object.compilerOptions.useEmptyPackage).toBeUndefined();
        });

        test('useEmptyPackage should default to false when undefined', () => {
            const config = createMockBuildConfig({
                frameworkMode: true,
                useEmptyPackage: undefined
            });
            const gen = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = gen.generateArkTSConfigFile(moduleInfo, false);
            expect(result.object.compilerOptions.useEmptyPackage).toBe(false);
        });

        test('useEmptyPackage set to false should be handled correctly', () => {
            const config = createMockBuildConfig({
                frameworkMode: true,
                useEmptyPackage: false
            });
            const gen = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = gen.generateArkTSConfigFile(moduleInfo, false);
            expect(result.object.compilerOptions.useEmptyPackage).toBe(false);
        });
    });

    describe('DeclgenEts2Ts Mode Processing', () => {
        test('should skip dynamic dependencies when enableDeclgenEts2Ts is true', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    '/dep/file': {
                        declPath: '/mock/dep.d.ets',
                        filePath: '/dep/file.ets',
                        ohmUrl: 'depModule/file'
                    }
                }
            }));

            const mockDepModule = { packageName: 'depModule', declFilesPath: '/mock/decl.json' } as any;
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, true);

            // Should only have stdlib, not dynamic deps
            const depKeys = Object.keys(result.dependencies);
            expect(depKeys.every(k => k.startsWith('std/') || k === 'escompat')).toBe(true);
            expect(result.dependencies['depModule//dep/file']).toBeUndefined();
        });

        test('should include dynamic dependencies when enableDeclgenEts2Ts is false', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    '/dep/index': {
                        declPath: '/mock/dep.d.ets',
                        filePath: '/dep/index.ets',
                        ohmUrl: 'depModule'
                    }
                }
            }));

            const mockDepModule = {
                packageName: 'depModule',
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/index'
            } as any;
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.dependencies['depModule//dep/index']).toBeDefined();
        });

        test('should include stdlib in both modes', () => {
            const moduleInfo = createMockModuleInfo();

            const result1 = generator.generateArkTSConfigFile(moduleInfo, true);
            const result2 = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result1.dependencies['std/core']).toBeDefined();
            expect(result2.dependencies['std/core']).toBeDefined();
        });
    });
});

/**
 * Stdlib Dependencies Management
 */
describe('ArkTSConfigGenerator - Standard Library Dependencies', () => {
    let mockLogger: any;
    let generator: ArkTSConfigGenerator;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
        const config = createMockBuildConfig();
        generator = ArkTSConfigGenerator.getInstance(config);
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Standard Library Inclusion', () => {
        test('should include all ETS standard libraries', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            const expectedLibs = [
                'std/core',
                'std/math',
                'std/math/consts',
                'std/containers',
                'std/interop/js',
                'std/time',
                'std/debug',
                'std/debug/concurrency',
                'std/testing',
                'std/concurrency',
                'std/annotations',
                'std/interop',
                'escompat'
            ];

            expectedLibs.forEach(lib => {
                expect(result.dependencies[lib]).toBeDefined();
                expect(result.dependencies[lib].language).toBe('ets');
                expect(result.dependencies[lib].ohmUrl).toBe(lib);
            });
        });

        test('all standard libraries should point to correct abc file', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['std/core'].path).toBe('/mock/panda/sdk/lib/etsstdlib.abc');
            expect(result.dependencies['std/math'].path).toBe('/mock/panda/sdk/lib/etsstdlib.abc');
            expect(result.dependencies['escompat'].path).toBe('/mock/panda/sdk/lib/etsstdlib.abc');
        });

        test('should include stdlib regardless of other dependencies', () => {
            const moduleInfo1 = createMockModuleInfo({ dynamicDependencyModules: new Map() });
            const result1 = generator.generateArkTSConfigFile(moduleInfo1, false);

            expect(result1.dependencies['std/core']).toBeDefined();
            expect(Object.keys(result1.dependencies).length).toBeGreaterThanOrEqual(13);
        });
    });

    describe('Standard Library Path Configuration', () => {
        test('should use pandaSdkPath from buildConfig', () => {
            ArkTSConfigGenerator.destroyInstance();
            const customConfig = createMockBuildConfig({
                pandaSdkPath: '/custom/panda/sdk'
            });
            const customGen = ArkTSConfigGenerator.getInstance(customConfig);
            const moduleInfo = createMockModuleInfo();

            const result = customGen.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['std/core'].path).toBe('/custom/panda/sdk/lib/etsstdlib.abc');
        });

        test('standard library dependencies should be ETS language', () => {
            const moduleInfo = createMockModuleInfo();
            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            Object.keys(result.dependencies)
                .filter(k => k.startsWith('std/') || k === 'escompat')
                .forEach(lib => {
                    expect(result.dependencies[lib].language).toBe('ets');
                });
        });
    });
});

/**
 * Path Mappings Management
 */
describe('ArkTSConfigGenerator - Path Mappings Management', () => {
    let mockLogger: any;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('System SDK Path Initialization', () => {
        test('should scan externalApiPaths', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue(['test.d.ets']);
            (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true, isDirectory: () => false });

            const config = createMockBuildConfig({
                externalApiPaths: ['/custom/api']
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['test']).toBeDefined();
        });

        test('should warn when path does not exist', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(false);

            const config = createMockBuildConfig({
                externalApiPaths: ['/nonexistent']
            });
            ArkTSConfigGenerator.getInstance(config);

            expect(mockLogger.printWarn).toHaveBeenCalledWith(
                expect.stringContaining('not exist')
            );
        });

        test('should handle multiple external API paths', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue([]);

            const config = createMockBuildConfig({
                externalApiPaths: ['/api1', '/api2', '/api3']
            });
            ArkTSConfigGenerator.getInstance(config);

            expect(fs.existsSync).toHaveBeenCalledWith('/api1');
            expect(fs.existsSync).toHaveBeenCalledWith('/api2');
            expect(fs.existsSync).toHaveBeenCalledWith('/api3');
        });

        test('should recursively scan subdirectories', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockImplementation((dir: string) => {
                if (dir.includes('subdir')) return ['file.d.ets'];
                return ['subdir'];
            });
            (fs.statSync as jest.Mock).mockImplementation((p: string) => {
                if (p.endsWith('subdir')) return { isFile: () => false, isDirectory: () => true };
                return { isFile: () => true, isDirectory: () => false };
            });

            const config = createMockBuildConfig({
                externalApiPaths: ['/api']
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['subdir.file']).toBeDefined();
        });
    });

    describe('Module Path Mapping', () => {
        test('should add module own path mapping', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/my/module'
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['myModule']).toEqual(['/my/module/src', '/my/module']);
        });

        test('empty package name should skip path mapping', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: '' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['']).toBeUndefined();
        });

        test('ArkTS 1.1 should skip path mapping', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'arkts11Pkg',
                language: LANGUAGE_VERSION.ARKTS_1_1
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['arkts11Pkg']).toBeUndefined();
        });

        test('should add entryFile path mapping with /Index suffix', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/my/module',
                entryFile: 'src/index.ets'  // relative path after getNormalizedEntryFile
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['myModule/Index']).toEqual(['/my/module/src/index.ets']);
        });
    });

    describe('Custom Path Mapping', () => {
        test('should merge paths from buildConfig', () => {
            const config = createMockBuildConfig({
                paths: {
                    'customAlias': ['/custom/path.d.ets'],
                    'anotherAlias': ['/another/path.d.ets']
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['customAlias']).toEqual(['/custom/path.d.ets']);
            expect(result.pathSection['anotherAlias']).toEqual(['/another/path.d.ets']);
        });

        test('should include system, custom and module paths simultaneously', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue(['sys.d.ets']);
            (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true, isDirectory: () => false });

            const config = createMockBuildConfig({
                externalApiPaths: ['/sys/api'],
                paths: { 'custom': ['/custom'] }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/module'
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['sys']).toBeDefined(); // system
            expect(result.pathSection['custom']).toEqual(['/custom']); // custom
            expect(result.pathSection['myModule']).toEqual(['/module/src', '/module']); // module
        });
    });
});

/**
 * Dynamic Dependencies Resolution
 */
describe('ArkTSConfigGenerator - Dynamic Dependencies Resolution', () => {
    let mockLogger: any;
    let generator: ArkTSConfigGenerator;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
        const config = createMockBuildConfig();
        generator = ArkTSConfigGenerator.getInstance(config);
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Decl File Parsing', () => {
        test('should load dependencies from decl file', () => {
            ArkTSConfigGenerator.destroyInstance();
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'lib/utils': {
                        declPath: '/mock/lib/utils.d.ets',
                        filePath: '/lib/utils.ets',
                        ohmUrl: 'libModule/utils'
                    }
                }
            }));

            const mockDepModule = createMockModuleInfo({
                entryFile: '/lib/util/entry.ets',
                packageName: 'libModule',
                declFilesPath: '/mock/decl.json'
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['libModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.object.compilerOptions.dependencies['libModule/lib/utils'].path).toBe('/mock/lib/utils.d.ets');
            expect(result.object.compilerOptions.dependencies['libModule/lib/utils'].ohmUrl).toBe('libModule/utils');
            expect(result.object.compilerOptions.dependencies['libModule/lib/utils'].language).toBe('js');
        });

        test('should create alias for entry file', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    '/lib/index': {
                        declPath: '/mock/lib/index.d.ets',
                        filePath: '/lib/index.ets',
                        ohmUrl: 'libModule'
                    }
                }
            }));

            const mockDepModule = {
                packageName: 'libModule',
                declFilesPath: '/mock/decl.json',
                entryFile: '/lib/index.ets'
            } as any;
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['libModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            // Should have two dependencies: one with full path, one with package name alias
            expect(result.dependencies['libModule']).toBeDefined();
            expect(result.dependencies['libModule//lib/index']).toBeDefined();
            expect(result.dependencies['libModule'].path).toBe('/mock/lib/index.d.ets');
        });

        test('should handle multiple files', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'lib/core': { declPath: '/mock/core.d.ets', filePath: '/lib/core.ets', ohmUrl: 'lib/core' },
                    'lib/utils': { declPath: '/mock/utils.d.ets', filePath: '/lib/utils.ets', ohmUrl: 'lib/utils' },
                    'lib/helpers': { declPath: '/mock/helpers.d.ets', filePath: '/lib/helpers.ets', ohmUrl: 'lib/helpers' }
                }
            }));

            const mockDepModule = createMockModuleInfo({
                entryFile: '/lib/util/entry.ets',
                packageName: 'libModule',
                declFilesPath: '/mock/decl.json'
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['lib', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.object.compilerOptions.dependencies['libModule/lib/core']).toBeDefined();
            expect(result.object.compilerOptions.dependencies['libModule/lib/utils']).toBeDefined();
            expect(result.object.compilerOptions.dependencies['libModule/lib/helpers']).toBeDefined();
        });
    });

    describe('Error Handling', () => {
        test('should warn when declFilesPath does not exist', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(false);

            const mockDepModule = createMockModuleInfo({
                entryFile: '/nonexistent/entry.ets',
                packageName: 'missingDep',
                declFilesPath: '/nonexistent.json'
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['missingDep', mockDepModule]])
            });

            generator.generateArkTSConfigFile(moduleInfo, false);

            expect(mockLogger.printWarn).toHaveBeenCalledWith(
                expect.stringContaining('decl file not found')
            );
        });

        test('should warn when declFilesPath is undefined', () => {
            const mockDepModule = createMockModuleInfo({
                entryFile: '/noDeclPath/entry.ets',
                packageName: 'noDeclPath',
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['noDeclPath', mockDepModule]])
            });

            generator.generateArkTSConfigFile(moduleInfo, false);

            expect(mockLogger.printWarn).toHaveBeenCalled();
        });

        test('one dependency failure should not affect other dependencies', () => {
            (fs.existsSync as jest.Mock).mockImplementation((p: string) => p.includes('valid'));
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'valid/file': {
                        declPath: '/mock/valid.d.ets',
                        filePath: '/valid/file.ets',
                        ohmUrl: 'validModule/file'
                    }
                }
            }));

            const mockInvalidDep = createMockModuleInfo({
                entryFile: '/lib/invalid/entry.ets',
                packageName: 'invalid',
                declFilesPath: '/invalid.json'
            });

            const mockValidDep = createMockModuleInfo({
                entryFile: '/lib/valid/entry.ets',
                packageName: 'validModule',
                declFilesPath: '/valid.json'
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['invalid', mockInvalidDep], ['validModule', mockValidDep]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(mockLogger.printWarn).toHaveBeenCalled();
            expect(result.object.compilerOptions.dependencies['validModule/valid/file']).toBeDefined();
        });
    });

    describe('Multi-Module Dependencies', () => {
        test('should handle multiple dependency modules', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockImplementation((path: string) => {
                if (path.includes('dep1')) {
                    return JSON.stringify({
                        files: { 'd1/f1': { declPath: '/d1.d.ets', filePath: '/d1/f1.ets', ohmUrl: 'd1' } }
                    });
                } else {
                    return JSON.stringify({
                        files: { 'd2/f2': { declPath: '/d2.d.ets', filePath: '/d2/f2.ets', ohmUrl: 'd2' } }
                    });
                }
            });
            const mockDep1 = createMockModuleInfo({
                packageName: 'dep1',
                declFilesPath: '/dep1.json',
                entryFile: '/dep1/entry.ts'
            });
            const mockDep2 = createMockModuleInfo({
                packageName: 'dep2',
                declFilesPath: '/dep2.json',
                entryFile: '/dep2/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                dynamicDependencyModules: new Map([['dep1', mockDep1], ['dep2', mockDep2]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.object.compilerOptions.dependencies['dep1/d1/f1']).toBeDefined();
            expect(result.object.compilerOptions.dependencies['dep2/d2/f2']).toBeDefined();
        });
    });
});

/**
 * Alias Processing
 */
describe('ArkTSConfigGenerator - Alias Processing', () => {
    let mockLogger: any;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
        (fs.existsSync as jest.Mock).mockReturnValue(true);
        ArkTSConfigGenerator.destroyInstance();

    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Static Aliases (@kit)', () => {
        test('should process @kit aliases as path mappings', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'MyKit': { originalAPIName: '@kit.Test', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['MyKit']).toEqual(['/mock/sdk/@kit.Test.d.ets']);
        });

        test('should skip when decl path is empty', () => {
            const getInteropFilePathByApi = require('../../../src/util/utils')
                .getInteropFilePathByApi as jest.Mock;
            getInteropFilePathByApi.mockReturnValueOnce('');

            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'BadKit': { originalAPIName: '@kit.Nonexistent', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['BadKit']).toBeUndefined();
        });
        test('should handle multiple @kit aliases', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'Kit1': { originalAPIName: '@kit.A', isStatic: false },
                        'Kit2': { originalAPIName: '@kit.B', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.object.compilerOptions.paths['Kit1']).toBeDefined();
            expect(result.object.compilerOptions.paths['Kit2']).toBeDefined();
        });

    });

    describe('Dynamic Aliases (@ohos, @system)', () => {
        test('should process @ohos aliases as dependencies', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'OhosAlias': { originalAPIName: '@ohos.app', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/@ohos.app']).toBeDefined();
            expect(result.dependencies['dynamic/@ohos.app'].alias).toContain('OhosAlias');
        });

        test('should process @system aliases as dependencies', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'SysAlias': { originalAPIName: '@system.router', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/@system.router']).toBeDefined();
            expect(result.dependencies['dynamic/@system.router'].alias).toContain('SysAlias');
        });

        test('should throw error when dynamic alias file does not exist', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(false);

            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'BadAlias': { originalAPIName: '@ohos.missing', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            expect(() => generator.generateArkTSConfigFile(moduleInfo, false))
                .toThrow();
        });
    });

    describe('Alias Filtering', () => {

        test('aliases with isStatic true should be skipped', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'StaticAlias': { originalAPIName: '@ohos.test', isStatic: true }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['@dynamic:@ohos.test']).toBeUndefined();
            expect(result.pathSection['StaticAlias']).toBeUndefined();
        });

        test('modules without aliasConfig should be handled normally', () => {
            const config = createMockBuildConfig({ aliasConfig: {} });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result).toBeInstanceOf(ArkTSConfig);
        });

        test('should handle multiple alias types for same module', () => {
            const config = createMockBuildConfig({
                aliasConfig: {
                    'testModule': {
                        'KitAlias': { originalAPIName: '@kit.A', isStatic: false },
                        'OhosAlias': { originalAPIName: '@ohos.b', isStatic: false },
                        'SysAlias': { originalAPIName: '@system.c', isStatic: false }
                    }
                }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'testModule' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['KitAlias']).toBeDefined();
            expect(result.dependencies['dynamic/@ohos.b']).toBeDefined();
            expect(result.dependencies['dynamic/@system.c']).toBeDefined();
        });
    });
});

/**
 * System SDK Processing
 */
describe('ArkTSConfigGenerator - System SDK Processing', () => {
    let mockLogger: any;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Dynamic SDK Path Processing', () => {
        test('should process files from interopSDKPaths', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue(['@ohos.test.d.ets']);
            (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true, isDirectory: () => false });

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/interop/sdk'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/@ohos.test']).toBeDefined();
        });

        test('should skip kits folder', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue([]);

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/sdk/kits'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            const dynamicDeps = Object.keys(result.dependencies).filter(k => k.startsWith('@dynamic:'));
            expect(dynamicDeps.length).toBe(0);
        });

        test('should add component prefix for component paths', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue(['button.d.ets']);
            (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true, isDirectory: () => false });

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/sdk/component'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['component/button']).toBeDefined();
        });

        test('should throw error when SDK path does not exist', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(false);

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/nonexistent/sdk'])
            });

            expect(() => ArkTSConfigGenerator.getInstance(config))
                .toThrow();
        });
    });

    describe('File Classification Processing', () => {
        test('API files should use dot separator', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockReturnValue(['@ohos.app.ability.d.ets']);
            (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true, isDirectory: () => false });

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/sdk'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/@ohos.app.ability']).toBeDefined();
        });

        test('non-API files should use slash separator', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockImplementation((dir: string) => {
                if (dir.includes('subdir')) return ['normal.d.ets'];
                return ['subdir'];
            });
            (fs.statSync as jest.Mock).mockImplementation((p: string) => {
                if (p.endsWith('subdir')) return { isFile: () => false, isDirectory: () => true };
                return { isFile: () => true, isDirectory: () => false };
            });

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/sdk'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/subdir/normal']).toBeDefined();
        });

        test('should recursively process nested directories', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readdirSync as jest.Mock).mockImplementation((dir: string) => {
                if (dir.includes('level2')) return ['file.d.ets'];
                if (dir.includes('level1')) return ['level2'];
                return ['level1'];
            });
            (fs.statSync as jest.Mock).mockImplementation((p: string) => {
                if (p.endsWith('.d.ets')) return { isFile: () => true, isDirectory: () => false };
                return { isFile: () => false, isDirectory: () => true };
            });

            const config = createMockBuildConfig({
                interopSDKPaths: new Set(['/sdk'])
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo();

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['dynamic/level1/level2/file']).toBeDefined();
        });
    });
});

/**
 * Integration Tests and Edge Cases
 */
describe('ArkTSConfigGenerator - Integration and Edge Cases', () => {
    let mockLogger: any;

    beforeEach(() => {
        mockLogger = setupBasicMocks();
        (fs.existsSync as jest.Mock).mockReturnValue(true);
        (fs.readdirSync as jest.Mock).mockReturnValue([]);
        (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => false, isDirectory: () => false });
    });

    afterEach(() => {
        ArkTSConfigGenerator.destroyInstance();
        jest.clearAllMocks();
    });

    describe('Complete Config Generation', () => {
        test('should generate complete config with all features enabled', () => {
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'dep/utils': {
                        declPath: '/mock/dep/utils.d.ets',
                        filePath: '/dep/utils.ets',
                        ohmUrl: 'depModule/utils'
                    }
                }
            }));

            const config = createMockBuildConfig({
                frameworkMode: true,
                useEmptyPackage: true,
                aliasConfig: {
                    'fullModule': {
                        'KitAlias': { originalAPIName: '@kit.Test', isStatic: false },
                        'OhosAlias': { originalAPIName: '@ohos.app', isStatic: false }
                    }
                },
                paths: { 'customPath': ['/custom'] }
            });
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/module.ets'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'fullModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['std/core']).toBeDefined();
            expect(result.dependencies['depModule/dep/utils']).toBeDefined();
            expect(result.pathSection['KitAlias']).toBeDefined();
            expect(result.dependencies['dynamic/@ohos.app']).toBeDefined();
            expect(result.pathSection['customPath']).toBeDefined();
            expect(result.pathSection['fullModule']).toBeDefined();
            expect(result.object.compilerOptions.useEmptyPackage).toBe(true);
        });
    });

    describe('Edge Cases', () => {
        test('empty package name should be handled correctly', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: '' });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.packageName).toBe('');
            expect(result.pathSection['']).toBeUndefined();
            expect(result.dependencies['std/core']).toBeDefined();
        });

        test('module with no dependencies should only contain stdlib', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ dynamicDependencyModules: new Map() });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.dependencies['std/core']).toBeDefined();
            const depKeys = Object.keys(result.dependencies);
            expect(depKeys.every(k => k.startsWith('std/') || k === 'escompat')).toBe(true);
        });

        test('should support generating configs for multiple modules', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            const config1 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'module1' }),
                false
            );
            const config2 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'module2' }),
                false
            );
            const config3 = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'module3' }),
                false
            );

            expect(config1.packageName).toBe('module1');
            expect(config2.packageName).toBe('module2');
            expect(config3.packageName).toBe('module3');

            expect(generator.getArktsConfigByPackageName('module1')).toBe(config1);
            expect(generator.getArktsConfigByPackageName('module2')).toBe(config2);
            expect(generator.getArktsConfigByPackageName('module3')).toBe(config3);
        });

        test('should support config merging', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            const base = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'base' }),
                false
            );
            const extended = generator.generateArkTSConfigFile(
                createMockModuleInfo({ packageName: 'extended' }),
                false
            );

            extended.mergeArktsConfig(base);

            expect(extended.pathSection['base']).toBeDefined();
            expect(extended.pathSection['extended']).toBeDefined();
            expect(extended.dependencies['std/core']).toBeDefined();
        });
    });

    describe('Concurrent and Repeated Generation', () => {
        test('regenerating same module should return different instances', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({ packageName: 'test' });

            const result1 = generator.generateArkTSConfigFile(moduleInfo, false);
            const result2 = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result1).not.toBe(result2);
            expect(result1.packageName).toBe(result2.packageName);
        });

        test('should handle large number of modules correctly', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);

            for (let i = 0; i < 100; i++) {
                const result = generator.generateArkTSConfigFile(
                    createMockModuleInfo({ packageName: `module${i}` }),
                    false
                );
                expect(result.packageName).toBe(`module${i}`);
            }

            expect(generator.getArktsConfigByPackageName('module0')).toBeDefined();
            expect(generator.getArktsConfigByPackageName('module50')).toBeDefined();
            expect(generator.getArktsConfigByPackageName('module99')).toBeDefined();
        });
    });

    describe('Test arktsconfig', () => {
        test('should add module source root mapping', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'myModule',
                moduleRootPath: '/my/module',
                sourceRoots: ['src', 'lib', 'test', 'official']
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['myModule']).toEqual(['/my/module/official',
                '/my/module/test', '/my/module/lib', '/my/module/src', '/my/module']);
        });

        test('should handle empty sourceRoots gracefully', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'emptyPkg',
                moduleRootPath: '/mod',
                sourceRoots: []
            });
        
            expect(() => generator.generateArkTSConfigFile(moduleInfo, false))
                .toThrow();
        });

        test('should reverse sourceRoots order in path mappings', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'testPkg',
                moduleRootPath: '/project/module',
                sourceRoots: ['first', 'second', 'third']
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);

            expect(result.pathSection['testPkg']).toEqual([
                '/project/module/third',
                '/project/module/second',
                '/project/module/first',
                '/project/module'
            ]);
        });

        test('should handle single source root correctly', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'singleRoot',
                moduleRootPath: '/app',
                sourceRoots: ['src/main']
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['singleRoot']).toEqual(['/app/src/main', '/app']);
        });

        test('should resolve absolute paths for source roots', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'absPkg',
                moduleRootPath: '/base/path',
                sourceRoots: ['rel/path1', './rel/path2', 'rel/path3']
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['absPkg']).toEqual([
                '/base/path/rel/path3',
                '/base/path/rel/path2',
                '/base/path/rel/path1',
                '/base/path'
            ]);
        });

        test('should handle sourceRoots with various path formats', () => {
            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const moduleInfo = createMockModuleInfo({
                packageName: 'mixedPaths',
                moduleRootPath: '/root',
                sourceRoots: ['./src', 'lib/', './test/', 'utils']
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            expect(result.pathSection['mixedPaths']).toHaveLength(5);

            expect(result.pathSection['mixedPaths']).toEqual([
                '/root/utils',
                '/root/test',
                '/root/lib',
                '/root/src',
                '/root'
            ]);
        });
    });

    describe('getDependencyKey with multiple sourceRoots', () => {
        test('should generate correct dependency keys with single source root', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/utils/helper': {
                        declPath: '/mock/helper.d.ets',
                        filePath: '/dep/src/utils/helper.ets',
                        ohmUrl: 'depModule/utils/helper'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // With sourceRoot 'src', the key should strip 'src/' prefix
            expect(result.dependencies['depModule/utils/helper']).toBeDefined();
            expect(result.dependencies['depModule/utils/helper'].ohmUrl).toBe('depModule/utils/helper');
        });

        test('should generate correct dependency keys with multiple source roots', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/main/ets/pages/Index': {
                        declPath: '/mock/Index.d.ets',
                        filePath: '/dep/src/main/ets/pages/Index.ets',
                        ohmUrl: 'depModule/pages/Index'
                    },
                    'lib/utils/common': {
                        declPath: '/mock/common.d.ets',
                        filePath: '/dep/lib/utils/common.ets',
                        ohmUrl: 'depModule/utils/common'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src/main', 'lib'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // sourceRoots are reversed, so 'lib' is checked first, then 'src/main'
            expect(result.dependencies['depModule/ets/pages/Index']).toBeDefined();
            expect(result.dependencies['depModule/utils/common']).toBeDefined();
        });

        test('should handle files not matching any source root', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'other/path/file': {
                        declPath: '/mock/file.d.ets',
                        filePath: '/dep/other/path/file.ets',
                        ohmUrl: 'depModule/other/path/file'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src', 'lib'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // File path doesn't match any source root, should keep full path
            expect(result.dependencies['depModule/other/path/file']).toBeDefined();
        });

        test('should strip ./ prefix from source roots when matching', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/main/component': {
                        declPath: '/mock/component.d.ets',
                        filePath: '/dep/src/main/component.ets',
                        ohmUrl: 'depModule/component'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['./src/main'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // ./ should be normalized and stripped correctly
            expect(result.dependencies['depModule/component']).toBeDefined();
        });

        test('should handle nested source roots correctly', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/main/ets/pages/Home': {
                        declPath: '/mock/Home.d.ets',
                        filePath: '/dep/src/main/ets/pages/Home.ets',
                        ohmUrl: 'depModule/pages/Home'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src', 'src/main', 'src/main/ets'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // Should match the most specific (longest) root first due to reversal
            // sourceRoots reversed: ['src/main/ets', 'src/main', 'src']
            expect(result.dependencies['depModule/pages/Home']).toBeDefined();
        });

        test('should filter out empty and dot source roots', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/test': {
                        declPath: '/mock/test.d.ets',
                        filePath: '/dep/src/test.ets',
                        ohmUrl: 'depModule/test'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src', '', '.', 'lib'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // Empty and '.' should be filtered out, only 'src' and 'lib' used
            expect(result.dependencies['depModule/test']).toBeDefined();
        });

        test('should handle Windows-style backslashes in source roots', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(JSON.stringify({
                files: {
                    'src/main/index': {
                        declPath: '/mock/index.d.ets',
                        filePath: '/dep/src/main/index.ets',
                        ohmUrl: 'depModule/index'
                    }
                }
            }));

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDepModule = createMockModuleInfo({
                packageName: 'depModule',
                moduleRootPath: '/dep',
                sourceRoots: ['src\\main'],
                declFilesPath: '/mock/decl.json',
                entryFile: '/dep/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([['depModule', mockDepModule]])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            // Backslashes should be normalized to forward slashes
            expect(result.dependencies['depModule/index']).toBeDefined();
        });

        test('should handle multiple dependency modules with different source roots', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockImplementation((path: string) => {
                if (path.includes('dep1')) {
                    return JSON.stringify({
                        files: {
                            'src/utils': {
                                declPath: '/mock/utils.d.ets',
                                filePath: '/dep1/src/utils.ets',
                                ohmUrl: 'dep1/utils'
                            }
                        }
                    });
                } else {
                    return JSON.stringify({
                        files: {
                            'lib/helpers': {
                                declPath: '/mock/helpers.d.ets',
                                filePath: '/dep2/lib/helpers.ets',
                                ohmUrl: 'dep2/helpers'
                            }
                        }
                    });
                }
            });

            const config = createMockBuildConfig();
            const generator = ArkTSConfigGenerator.getInstance(config);
            const mockDep1 = createMockModuleInfo({
                packageName: 'dep1',
                moduleRootPath: '/dep1',
                sourceRoots: ['src'],
                declFilesPath: '/dep1/decl.json',
                entryFile: '/dep1/entry.ts'
            });
            const mockDep2 = createMockModuleInfo({
                packageName: 'dep2',
                moduleRootPath: '/dep2',
                sourceRoots: ['lib'],
                declFilesPath: '/dep2/decl.json',
                entryFile: '/dep2/entry.ts'
            });
            const moduleInfo = createMockModuleInfo({
                packageName: 'mainModule',
                dynamicDependencyModules: new Map([
                    ['dep1', mockDep1],
                    ['dep2', mockDep2]
                ])
            });

            const result = generator.generateArkTSConfigFile(moduleInfo, false);
            
            expect(result.dependencies['dep1/utils']).toBeDefined();
            expect(result.dependencies['dep2/helpers']).toBeDefined();
        });
    });
});
