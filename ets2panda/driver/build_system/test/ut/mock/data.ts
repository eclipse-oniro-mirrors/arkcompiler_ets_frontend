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

import {
    BUILD_MODE,
    BUILD_TYPE,
    BuildConfig,
    ES2PANDA_MODE,
    ModuleInfo,
    OHOS_MODULE_TYPE
} from '../../../src/types';

import {
    ILogger,
    LoggerGetter,
} from '../../../src/logger';

export function getMockedBuildConfig(): BuildConfig {
    return {
        // BuildBaseConfig
        buildType: BUILD_TYPE.BUILD,
        buildMode: BUILD_MODE.DEBUG,
        es2pandaMode: ES2PANDA_MODE.RUN,
        isBuildConfigModified: undefined,
        recordType: undefined,

        // DeclGenConfig
        enableDeclgenEts2Ts: false,
        declgenV1OutPath: "",
        declgenV2OutPath: "",
        declgenBridgeCodePath: "",
        skipDeclCheck: undefined,

        // LoggerConfig
        getHvigorConsoleLogger: undefined,

        // ModuleConfig
        packageName: "test",
        moduleType: OHOS_MODULE_TYPE.HAR,
        moduleRootPath: "/test/path",
        sourceRoots: ["./"],
        byteCodeHar: false,
        entryFile: "index.ets",

        // PathConfig
        loaderOutPath: "./dist",
        cachePath: "./dist/cache",
        buildSdkPath: "/path/to/sdk",
        pandaSdkPath: undefined,
        pandaStdlibPath: undefined,
        externalApiPaths: [],
        abcLinkerPath: undefined,
        dependencyAnalyzerPath: undefined,
        sdkAliasConfigPaths: undefined,
        sdkAliasMap: new Map(),
        interopSDKPaths: new Set(),
        interopApiPaths: [],
        projectRootPath: '',

        // FrameworkConfig
        frameworkMode: undefined,
        useEmptyPackage: undefined,

        // BuildConfig
        plugins: {},
        paths: {},
        compileFiles: ["test/path/index.ets"],
        dependencyModuleList: [],
        aliasConfig: {},
        dependentModuleList: [],
        hasMainModule: false
    };
}

export function getMockLoggerGetter(spyMock?: jest.Mock): LoggerGetter {
    return (): ILogger => {
        return {
            printInfo: spyMock ?? jest.fn(),
            printWarn: spyMock ?? jest.fn(),
            printDebug: spyMock ?? jest.fn(),
            printError: spyMock ?? jest.fn(),
            printErrorAndExit: spyMock ?? jest.fn(),
        }
    }
}


export const moduleInfoWithNullSourceRoots: ModuleInfo = {
    isMainModule: true,
    packageName: 'test-pkg',
    moduleRootPath: '/tmp/test-module',
    moduleType: 'type',
    sourceRoots: [],
    entryFile: 'index.ets',
    arktsConfigFile: 'arktsconfig.json',
    declgenV1OutPath: "",
    declgenV2OutPath: "",
    declgenBridgeCodePath: "",
    byteCodeHar: false,
    dependencies: [],
    staticDependencyModules: new Map(),
    dynamicDependencyModules: new Map(),
};

export const moduleInfoWithFalseEts2Ts: ModuleInfo = {
    isMainModule: true,
    packageName: 'test-pkg',
    moduleRootPath: '/tmp/test-module',
    moduleType: 'type',
    sourceRoots: ['/src/moduleA'],
    entryFile: 'index.ets',
    arktsConfigFile: 'arktsconfig.json',
    declgenV1OutPath: "",
    declgenV2OutPath: "",
    declgenBridgeCodePath: "",
    byteCodeHar: false,
    dependencies: [],
    staticDependencyModules: new Map(),
    dynamicDependencyModules: new Map(),
};

export const moduleInfo: ModuleInfo = {
    isMainModule: false,
    packageName: 'moduleA',
    moduleRootPath: '/src/moduleA',
    moduleType: 'feature',
    sourceRoots: ['/src/moduleA'],
    entryFile: '/src/moduleA/index.ts',
    arktsConfigFile: '/path/to/moduleA/arktsConfig.json',
    dependencies: [],
    staticDependencyModules: new Map(),
    dynamicDependencyModules: new Map(),
    declgenV1OutPath: '/path/to/moduleA/declgen/v1',
    declgenV2OutPath: '/path/to/moduleA/declgen/v2',
    declgenBridgeCodePath: '/path/to/moduleA/bridge/code',
    language: "1.2",
    declFilesPath: '/path/to/moduleA/declFiles',
    byteCodeHar: true,
    abcPath: '/path/to/moduleA/abc/file.abc'
};

export const mockModuleInfos: Map<string, ModuleInfo> = new Map([
    [
        'moduleA',
        {
            ...moduleInfoWithFalseEts2Ts
        },
    ],
]);

describe('mockData', () => {
    it('should load correctly', () => {
        const mock = require('./data');
        expect(mock).toBeDefined();
    });
});
