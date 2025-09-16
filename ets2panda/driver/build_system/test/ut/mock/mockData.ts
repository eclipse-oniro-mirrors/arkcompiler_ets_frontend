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

import { ModuleInfo } from '../../../src/types';

export const moduleInfoWithNullSourceRoots: ModuleInfo = {
    isMainModule: true,
    packageName: 'test-pkg',
    moduleRootPath: '/tmp/test-module',
    moduleType: 'type',
    sourceRoots: [],
    entryFile: 'index.ets',
    arktsConfigFile: 'arktsconfig.json',
    compileFileInfos: [],
    declgenV1OutPath: undefined,
    declgenV2OutPath: undefined,
    declgenBridgeCodePath: undefined,
    byteCodeHar: false,
    staticDepModuleInfos: new Map(),
    dynamicDepModuleInfos: new Map()
};

export const moduleInfoWithFalseEts2Ts: ModuleInfo = {
    isMainModule: true,
    packageName: 'test-pkg',
    moduleRootPath: '/tmp/test-module',
    moduleType: 'type',
    sourceRoots: ['/src/moduleA'],
    entryFile: 'index.ets',
    arktsConfigFile: 'arktsconfig.json',
    compileFileInfos: [],
    declgenV1OutPath: undefined,
    declgenV2OutPath: undefined,
    declgenBridgeCodePath: undefined,
    byteCodeHar: false,
    staticDepModuleInfos: new Map(),
    dynamicDepModuleInfos: new Map(),
};

export const moduleInfo: ModuleInfo = {
    isMainModule: false,
    packageName: 'moduleA',
    moduleRootPath: '/src/moduleA',
    moduleType: 'feature',
    sourceRoots: ['/src/moduleA'],
    entryFile: '/src/moduleA/index.ts',
    arktsConfigFile: '/path/to/moduleA/arktsConfig.json',
    compileFileInfos: [],
    dynamicDepModuleInfos: new Map<string, ModuleInfo>(),
    staticDepModuleInfos: new Map<string, ModuleInfo>(),
    declgenV1OutPath: '/path/to/moduleA/declgen/v1',
    declgenV2OutPath: '/path/to/moduleA/declgen/v2',
    declgenBridgeCodePath: '/path/to/moduleA/bridge/code',
    language: "1.2",
    declFilesPath: '/path/to/moduleA/declFiles',
    dependencies: [],
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

export let mockLogger: mockLogger = {
    printErrorAndExit: jest.fn(() => { throw new Error('Exit with error.'); }),
    printWarn: jest.fn(),
};
interface mockLogger {
    printErrorAndExit: jest.Mock;
    printWarn: jest.Mock;
}
