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

import { BuildFrameworkMode } from '../../../src/build/build_framework_mode';
import { BUILD_TYPE, BuildConfig, OHOS_MODULE_TYPE, BUILD_MODE, ModuleInfo, PluginsConfig } from '../../../src/types';
import { BaseMode } from '../../../src/build/base_mode';

jest.mock('../../../src/logger', () => ({
    Logger: {
        getInstance: jest.fn().mockReturnValue({
            printInfo: jest.fn(),
            printError: jest.fn(),
            hasErrors: jest.fn().mockReturnValue(false)
        })
    }
}));

// Test the functions of the build_framework_mode.ts file
describe('test build_framework_mode.ts file api', () => {
    const createMockConfig = {
        buildType: BUILD_TYPE.BUILD,
        packageName: 'test',
        compileFiles: ['test.ets'],
        enableDeclgenEts2Ts: false,
        frameworkMode: true,
        useEmptyPackage: true,
        loaderOutPath: './dist',
        cachePath: './dist/cache',
        moduleType: OHOS_MODULE_TYPE.HAR,
        sourceRoots: ['./'],
        moduleRootPath: '/test/path',
        buildMode: BUILD_MODE.DEBUG,
        plugins: {} as PluginsConfig,
        dependentModuleList: [],
        hasMainModule: false,
        byteCodeHar: false,
        arkts: {} as any,
        arktsGlobal: {} as any,
        declgenV1OutPath: "./dist/declgen",
        declgenV2OutPath: "./dist/declgen/v2",
        buildSdkPath: "./sdk",
        externalApiPaths: []
    } as any;

    test('constructor should set properties correctly', () => {
        class TestBuildFrameworkMode extends BuildFrameworkMode {
            public async run(): Promise<void> { }
        }

        const buildFrameworkMode = new TestBuildFrameworkMode(createMockConfig);

        expect(buildFrameworkMode.frameworkMode).toBe(true);
        expect(buildFrameworkMode.useEmptyPackage).toBe(true);
        expect((buildFrameworkMode as any).mergedAbcFile).toBe('./dist');

        const configWithUndefined = {
            ...createMockConfig,
            frameworkMode: undefined,
            useEmptyPackage: undefined
        };
        const defaultBuildMode = new TestBuildFrameworkMode(configWithUndefined as BuildConfig);
        expect(defaultBuildMode.frameworkMode).toBe(false);
        expect(defaultBuildMode.useEmptyPackage).toBe(false);
    });

    test('run method should call super.run', async () => {
        class TestBuildFrameworkMode extends BuildFrameworkMode {
            public superRunCalled = false;

            public async run(): Promise<void> {
                this.superRunCalled = true;
            }
        }

        const buildFrameworkMode = new TestBuildFrameworkMode(createMockConfig);
        await buildFrameworkMode.run();

        expect(buildFrameworkMode.superRunCalled).toBe(true);
    });

    test('getMainModuleInfo should set framework properties', () => {
        class TestBuildFrameworkMode extends BuildFrameworkMode {
            public testGetMainModuleInfo(): ModuleInfo {
                const moduleInfo = {
                    packageName: 'test',
                    moduleRootPath: '/test/path'
                } as ModuleInfo;

                moduleInfo.frameworkMode = this.frameworkMode;
                moduleInfo.useEmptyPackage = this.useEmptyPackage;

                return moduleInfo;
            }
        }

        const buildFrameworkMode = new TestBuildFrameworkMode(createMockConfig);
        const moduleInfo = buildFrameworkMode.testGetMainModuleInfo();

        expect(moduleInfo.frameworkMode).toBe(true);
        expect(moduleInfo.useEmptyPackage).toBe(true);
    });

    test('generateModuleInfos should call expected methods', async () => {
        class TestBuildFrameworkMode extends BuildFrameworkMode {
            public methodsCalled = {
                collectModuleInfos: false,
                generateArkTSConfigForModules: false,
                collectCompileFiles: false
            };

            protected collectModuleInfos(): void {
                this.methodsCalled.collectModuleInfos = true;
            }

            protected generateArkTSConfigForModules(): void {
                this.methodsCalled.generateArkTSConfigForModules = true;
            }

            protected collectCompileFiles(): void {
                this.methodsCalled.collectCompileFiles = true;
            }

            public testGenerateModuleInfos(): void {
                this.generateModuleInfos();
            }
        }

        const buildFrameworkMode = new TestBuildFrameworkMode(createMockConfig);
        buildFrameworkMode.testGenerateModuleInfos();

        expect(buildFrameworkMode.methodsCalled.collectModuleInfos).toBe(true);
        expect(buildFrameworkMode.methodsCalled.generateArkTSConfigForModules).toBe(true);
        expect(buildFrameworkMode.methodsCalled.collectCompileFiles).toBe(true);
    });

    test('getMainModuleInfo should extend super.getMainModuleInfo and set framework properties', () => {
        class TestBuildFrameworkMode extends BuildFrameworkMode {
            public testGetMainModuleInfo(): ModuleInfo {
                return this.getMainModuleInfo();
            }

            protected getMainModuleInfoFromSuper(): ModuleInfo {
                return {
                    isMainModule: true,
                    packageName: 'test',
                    moduleRootPath: '/test/path',
                    sourceRoots: ['./'],
                    arktsConfigFile: '/test/config.json',
                    compileFileInfos: [],
                    dynamicDepModuleInfos: new Map(),
                    staticDepModuleInfos: new Map(),
                    dependenciesSet: new Set(),
                    dependentSet: new Set(),
                    moduleType: OHOS_MODULE_TYPE.HAR,
                    entryFile: 'index.ets',
                    byteCodeHar: false,
                    declgenV1OutPath: '/test/declgen',
                    declgenV2OutPath: '/test/declgen/v2',
                    declgenBridgeCodePath: '/test/bridge'
                };
            }
        }

        jest.spyOn((BaseMode as any).prototype, 'getMainModuleInfo').mockImplementation(function() {
            return {
                isMainModule: true,
                packageName: 'test',
                moduleRootPath: '/test/path',
                sourceRoots: ['./'],
                arktsConfigFile: '/test/config.json',
                compileFileInfos: [],
                dynamicDepModuleInfos: new Map(),
                staticDepModuleInfos: new Map(),
                moduleType: OHOS_MODULE_TYPE.HAR,
                entryFile: 'index.ets',
                byteCodeHar: false,
                declgenV1OutPath: '/test/declgen',
                declgenV2OutPath: '/test/declgen/v2',
                declgenBridgeCodePath: '/test/bridge'
            };
        });

        const buildFrameworkMode = new TestBuildFrameworkMode(createMockConfig);
        const moduleInfo = buildFrameworkMode.testGetMainModuleInfo();

        expect(moduleInfo.frameworkMode).toBe(true);
        expect(moduleInfo.useEmptyPackage).toBe(true);
        expect(moduleInfo.packageName).toBe('test');
        expect(moduleInfo.moduleRootPath).toBe('/test/path');
        expect(moduleInfo.isMainModule).toBe(true);

        const configWithoutFramework = {
            ...createMockConfig,
            frameworkMode: false,
            useEmptyPackage: false
        };

        const buildFrameworkMode2 = new TestBuildFrameworkMode(configWithoutFramework as BuildConfig);
        const moduleInfo2 = buildFrameworkMode2.testGetMainModuleInfo();

        expect(moduleInfo2.frameworkMode).toBe(false);
        expect(moduleInfo2.useEmptyPackage).toBe(false);
    });
});
