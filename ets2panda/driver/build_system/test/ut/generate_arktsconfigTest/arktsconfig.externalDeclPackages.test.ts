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
import { BuildConfig, DependencyModuleConfig, ModuleInfo } from '../../../src/types';

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
        dependencyModuleList: [],
        ...overrides
    } as BuildConfig;
}

function createMockDependencyModuleConfig(
    overrides: Partial<DependencyModuleConfig> = {}
): DependencyModuleConfig {
    return {
        packageName: 'libhar',
        moduleName: 'libhar',
        moduleType: 'har',
        modulePath: '/mock/project/libhar',
        sourceRoots: ['src'],
        entryFile: 'Index.ets',
        language: 'ets',
        dependencies: [],
        ...overrides
    } as DependencyModuleConfig;
}

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        packageName: 'entry',
        moduleRootPath: '/mock/project/entry',
        sourceRoots: ['src'],
        arktsConfigFile: '/mock/arktsconfig.json',
        compileFileInfos: [],
        dynamicDependencyModules: new Map(),
        staticDependencyModules: new Map(),
        entryFile: 'Index.ets',
        dependencies: [],
        ...overrides
    } as ModuleInfo;
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

function createArktsConfigWithPaths(
    moduleInfo: ModuleInfo,
    paths: Record<string, string[]>
): ArkTSConfig {
    const arktsConfig = new ArkTSConfig(moduleInfo, '/mock/cache', '/mock/project', '/mock/declgen');
    arktsConfig.addPathMappings(paths);
    return arktsConfig;
}

describe('ArkTSConfig - externalDeclPackages accessor', () => {
    beforeEach(() => {
        setupBasicMocks();
    });

    afterEach(() => {
        jest.clearAllMocks();
    });

    test('should return undefined by default', () => {
        const arktsConfig = createArktsConfigWithPaths(createMockModuleInfo(), {});

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
        expect(arktsConfig.compilerOptions.externalDeclPackages).toBeUndefined();
    });

    test('setter should store value and getter should return it', () => {
        const arktsConfig = createArktsConfigWithPaths(createMockModuleInfo(), {});
        const externalDeclPackages = {
            'libhar': { paths: ['/mock/project/libhar'] }
        };

        arktsConfig.externalDeclPackages = externalDeclPackages;

        expect(arktsConfig.externalDeclPackages).toEqual(externalDeclPackages);
        expect(arktsConfig.object.compilerOptions.externalDeclPackages).toEqual(externalDeclPackages);
    });

    test('setter should accept undefined to clear the value', () => {
        const arktsConfig = createArktsConfigWithPaths(createMockModuleInfo(), {});
        arktsConfig.externalDeclPackages = { 'libhar': { paths: ['/mock/project/libhar'] } };

        arktsConfig.externalDeclPackages = undefined;

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });
});

describe('ArkTSConfigGenerator - addExternalDeclPackages', () => {
    beforeEach(() => {
        setupBasicMocks();
    });

    afterEach(() => {
        jest.clearAllMocks();
    });

    test('should do nothing when moduleInfo has no dependencies', () => {
        const generator = new ArkTSConfigGenerator(createMockBuildConfig());
        const moduleInfo = createMockModuleInfo({ dependencies: [] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should do nothing when moduleInfo dependencies is undefined', () => {
        const generator = new ArkTSConfigGenerator(createMockBuildConfig());
        const moduleInfo = createMockModuleInfo({ dependencies: undefined });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo as any, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should skip dependency that is not present in dependencyModuleList', () => {
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: []
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should skip source-only dependency without byteCodeHar and abcPath', () => {
        const dep = createMockDependencyModuleConfig({ packageName: 'libhar' });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should collect path keys for dependency with byteCodeHar and abcPath', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: true,
            abcPath: '/mock/output/libhar.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'entry': ['/mock/project/entry'],
            'libhar': ['/mock/project/libhar'],
            'libhar/src': ['/mock/project/libhar/src'],
            'libhar/Index': ['/mock/project/libhar/Index.ets'],
            'otherlib': ['/mock/project/otherlib']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toEqual({
            'libhar': { paths: ['/mock/project/libhar'] },
            'libhar/src': { paths: ['/mock/project/libhar/src'] },
            'libhar/Index': { paths: ['/mock/project/libhar/Index.ets'] }
        });
    });

    test('should skip dependency marked only by byteCodeHar', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhsp',
            byteCodeHar: true
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhsp'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhsp': ['/mock/project/libhsp'],
            'libhsp/src': ['/mock/project/libhsp/src']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should skip dependency carrying only abcPath', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            abcPath: '/mock/output/libhar.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should collect path keys for external hsp with byteCodeHar false and abcPath', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhsp',
            byteCodeHar: false,
            abcPath: '/mock/output/libhsp.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhsp'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhsp': ['/mock/project/libhsp'],
            'libhsp/src': ['/mock/project/libhsp/src']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toEqual({
            'libhsp': { paths: ['/mock/project/libhsp'] },
            'libhsp/src': { paths: ['/mock/project/libhsp/src'] }
        });
    });

    test('should skip hsp dependency with byteCodeHar false when abcPath is missing', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: false
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should not collect path keys of packages sharing a common prefix', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: true,
            abcPath: '/mock/output/libhar.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar'],
            'libhar10': ['/mock/project/libhar10'],
            'libhar10/src': ['/mock/project/libhar10/src'],
            'libharx': ['/mock/project/libharx'],
            'libha': ['/mock/project/libha']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toEqual({
            'libhar': { paths: ['/mock/project/libhar'] }
        });
    });

    test('should not set externalDeclPackages when no path key matches', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: true,
            abcPath: '/mock/output/libhar.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'entry': ['/mock/project/entry'],
            'otherlib': ['/mock/project/otherlib']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toBeUndefined();
    });

    test('should copy path arrays instead of sharing references', () => {
        const dep = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: true,
            abcPath: '/mock/output/libhar.abc'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [dep]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        const collected = arktsConfig.externalDeclPackages!['libhar'].paths;
        expect(collected).toEqual(['/mock/project/libhar']);
        expect(collected).not.toBe(arktsConfig.pathSection['libhar']);
    });

    test('should collect path keys for multiple dependencies', () => {
        const depHar = createMockDependencyModuleConfig({
            packageName: 'libhar',
            byteCodeHar: true,
            abcPath: '/mock/output/libhar.abc'
        });
        const depHsp = createMockDependencyModuleConfig({
            packageName: 'libhsp',
            byteCodeHar: false,
            abcPath: '/mock/output/libhsp.abc'
        });
        const depSource = createMockDependencyModuleConfig({
            packageName: 'sourcemodule'
        });
        const generator = new ArkTSConfigGenerator(createMockBuildConfig({
            dependencyModuleList: [depHar, depHsp, depSource]
        }));
        const moduleInfo = createMockModuleInfo({ dependencies: ['libhar', 'libhsp', 'sourcemodule'] });
        const arktsConfig = createArktsConfigWithPaths(moduleInfo, {
            'libhar': ['/mock/project/libhar'],
            'libhsp': ['/mock/project/libhsp'],
            'libhsp/src': ['/mock/project/libhsp/src'],
            'sourcemodule': ['/mock/project/sourcemodule']
        });

        generator.addExternalDeclPackages(moduleInfo, arktsConfig);

        expect(arktsConfig.externalDeclPackages).toEqual({
            'libhar': { paths: ['/mock/project/libhar'] },
            'libhsp': { paths: ['/mock/project/libhsp'] },
            'libhsp/src': { paths: ['/mock/project/libhsp/src'] }
        });
    });
});
