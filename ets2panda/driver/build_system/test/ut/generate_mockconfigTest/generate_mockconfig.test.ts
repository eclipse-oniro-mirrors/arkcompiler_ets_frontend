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

import fs from 'fs';
import os from 'os';
import path from 'path';
import { MockConfigGenerator } from '../../../src/build/generate_mockconfig';
import { BuildConfig, DependencyModuleConfig } from '../../../src/types';

// The mocked logger replaces the `Logger` class with an instance-like object at runtime;
// reach it via `require` (untyped) so we can assert on its methods.
const MockLogger: any = require('../../../src/logger').Logger;

/**
 * Only the logger is mocked so that `printErrorAndExit` does not really call `process.exit`.
 * `fs` / `path` / `json5` are real — the generator operates on a real temp fixture tree,
 * which also exercises the low-level `readFirstLineSync` (openSync/readSync/closeSync) path.
 */
jest.mock('../../../src/logger', () => {
    const mLogger = {
        printError: jest.fn(),
        printInfo: jest.fn(),
        printErrorAndExit: jest.fn(),
        getInstance: jest.fn((): any => mLogger)
    } as any;
    return {
        Logger: mLogger,
        LogDataFactory: {
            newInstance: jest.fn((code: string, description: string) => ({ code, description }))
        }
    };
});

const USE_STATIC_SINGLE = `'use static'\nexport function foo() {}\n`;
const USE_STATIC_DOUBLE = `"use static";\nexport function bar() {}\n`;
const NO_USE_STATIC = `export function baz() {}\n`;

let tmpDir: string;
let moduleRootPath: string;

function writeUseStaticFile(filePath: string, content: string = USE_STATIC_SINGLE): void {
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, content);
}

function setupFixtures(): void {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'mock-cfg-'));
    moduleRootPath = path.join(tmpDir, 'entry');

    // key source files
    writeUseStaticFile(path.join(moduleRootPath, 'src', 'main', 'ets', 'util.ets'));
    writeUseStaticFile(path.join(tmpDir, 'bar.ets'));                 // target of relative key "./bar"
    // mock sources
    writeUseStaticFile(path.join(moduleRootPath, 'mocks', 'ohos_mock.ets'));
    writeUseStaticFile(path.join(moduleRootPath, 'mocks', 'util_mock.ets'));
    writeUseStaticFile(path.join(moduleRootPath, 'mocks', 'rel_mock.ets'));
    writeUseStaticFile(path.join(moduleRootPath, 'mocks', 'dq_mock.ets'), USE_STATIC_DOUBLE);
    // negative fixture: mock without 'use static'
    writeUseStaticFile(path.join(moduleRootPath, 'mocks', 'bad_mock.ets'), NO_USE_STATIC);
}

function makeBuildConfig(configJson5: string, overrides: Partial<BuildConfig> = {}): BuildConfig {
    const configPath = path.join(tmpDir, 'mock-config-static.json5');
    fs.writeFileSync(configPath, configJson5);
    return {
        mockParams: { mockConfigPath: configPath },
        moduleRootPath,
        ...overrides
    } as BuildConfig;
}

beforeEach(() => {
    jest.clearAllMocks();
    setupFixtures();
});

afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true, force: true });
});

describe('MockConfigGenerator construction', () => {
    test('each construction creates an independent instance (not shared)', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/ohos_mock.ets" } }`);
        const a = new MockConfigGenerator(bc);
        const b = new MockConfigGenerator(bc);
        expect(a).not.toBe(b);
        // mutating one must not affect the other
        expect(a.getMockConfigInfo()).not.toBe(b.getMockConfigInfo());
    });
});

describe('MockConfigGenerator mockParams getter', () => {
    test('exposes the buildConfig mockParams', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/ohos_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc);
        expect(gen.mockParams.mockConfigPath).toBe(bc.mockParams!.mockConfigPath);
    });
});

describe('MockConfigGenerator collectMockConfigInfo - valid', () => {
    test('system API key (@ohos/@arkts/@kit) is kept as-is with an absolute mock source', () => {
        for (const prefix of ['@ohos', '@arkts', '@kit']) {
            const bc = makeBuildConfig(`{ "${prefix}.api": { source: "mocks/ohos_mock.ets" } }`);
            const gen = new MockConfigGenerator(bc);
            gen.collectMockConfigInfo();
            const info = gen.getMockConfigInfo();
            expect(info[`${prefix}.api`]).toBeDefined();
            expect(info[`${prefix}.api`].source).toBe(path.join(moduleRootPath, 'mocks', 'ohos_mock.ets'));
        }
    });

    test('relative key is resolved to an absolute key', () => {
        const bc = makeBuildConfig(`{ "./bar": { source: "mocks/rel_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'rel_mock.ets');
        const absKey = path.join(tmpDir, 'bar.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
    });

    test('source-file key (no prefix) resolves to src/main/ets', () => {
        const bc = makeBuildConfig(`{ "util": { source: "mocks/util_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'util_mock.ets');
        const absKey = path.join(moduleRootPath, 'src', 'main', 'ets', 'util.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
    });

    test('double-quoted "use static" first line is rejected (only single-quote \'use static\' accepted)', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/dq_mock.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });
});

describe('MockConfigGenerator collectMockConfigInfo - invalid', () => {
    test('entry missing a string source field reports an error', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": {} }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });

    test('mock source without a use-static first line reports an error', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/bad_mock.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });

    test('key file without a use-static first line reports an error', () => {
        writeUseStaticFile(path.join(moduleRootPath, 'src', 'main', 'ets', 'nostatic.ets'), NO_USE_STATIC);
        const bc = makeBuildConfig(`{ "nostatic": { source: "mocks/ohos_mock.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });

    test('non-existent mock source reports an error', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/nonexistent.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });

    test('key that is not a system API / relative / dependency / source file reports an error', () => {
        const bc = makeBuildConfig(`{ "no_such_key": { source: "mocks/ohos_mock.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });

    test('relative key whose target file does not exist reports an error', () => {
        const bc = makeBuildConfig(`{ "./missing": { source: "mocks/ohos_mock.ets" } }`);
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });
});

describe('MockConfigGenerator validateKeyFile - dependency branch', () => {
    function makeDep(modulePath: string): DependencyModuleConfig {
        return {
            packageName: '@lib/har',
            moduleName: 'har',
            moduleType: 'har',
            modulePath,
            sourceRoots: [],
            entryFile: '',
            language: 'ets'
        } as DependencyModuleConfig;
    }

    test('key equal to a dependency package name is kept as-is', () => {
        const depPath = path.join(tmpDir, 'dep');
        writeUseStaticFile(path.join(depPath, 'index.ets'));
        const bc = makeBuildConfig(`{ "@lib/har": { source: "mocks/ohos_mock.ets" } }`, {
            dependencyModuleList: [makeDep(depPath)]
        });
        const gen = new MockConfigGenerator(bc);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();
        expect(MockLogger.printErrorAndExit).not.toHaveBeenCalled();
        expect(info['@lib/har']).toBeDefined();
        expect(info['@lib/har'].source).toBe(path.join(moduleRootPath, 'mocks', 'ohos_mock.ets'));
    });

    test('key "<pkg>/<sub>" resolves against the dependency module path', () => {
        const depPath = path.join(tmpDir, 'dep');
        writeUseStaticFile(path.join(depPath, 'sub.ets'));
        const bc = makeBuildConfig(`{ "@lib/har/sub": { source: "mocks/ohos_mock.ets" } }`, {
            dependencyModuleList: [makeDep(depPath)]
        });
        const gen = new MockConfigGenerator(bc);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'ohos_mock.ets');
        // resolved dependency path, pointing to the mock source
        const absKey = path.join(depPath, 'sub.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
        expect(MockLogger.printErrorAndExit).not.toHaveBeenCalled();
    });

    test('dependency sub-path that does not exist reports an error', () => {
        const depPath = path.join(tmpDir, 'dep');
        // dep/sub.ets intentionally NOT created
        const bc = makeBuildConfig(`{ "@lib/har/sub": { source: "mocks/ohos_mock.ets" } }`, {
            dependencyModuleList: [makeDep(depPath)]
        });
        new MockConfigGenerator(bc).collectMockConfigInfo();
        expect(MockLogger.printErrorAndExit).toHaveBeenCalled();
    });
});

describe('MockConfigGenerator getMockConfigInfo', () => {
    test('returns an empty map before collectMockConfigInfo and a populated map after', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/ohos_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc);
        expect(Object.keys(gen.getMockConfigInfo())).toHaveLength(0);

        gen.collectMockConfigInfo();
        expect(Object.keys(gen.getMockConfigInfo())).toHaveLength(1);
    });
});
