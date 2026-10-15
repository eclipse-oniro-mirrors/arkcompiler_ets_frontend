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
import { DriverError } from '../../../src/util/error';

/**
 * `fs` / `path` / `json5` are real — the generator operates on a real temp fixture tree,
 * which also exercises the low-level `readFirstLineSync` (openSync/readSync/closeSync) path.
 */

const USE_STATIC_SINGLE = `'use static'\nexport function foo() {}\n`;
const USE_STATIC_DOUBLE = `"use static";\nexport function bar() {}\n`;
const NO_USE_STATIC = `export function baz() {}\n`;

// Mirrors the `systemApiPathSection` the ArkTSConfigGenerator builds from the SDK
// (api / arkts / kits declaration files): the keys are import specifiers.
const SYSTEM_API_SECTION: Record<string, string[]> = {
    '@ohos.foo': ['/sdk/api/@ohos.foo.d.ets'],
    '@ohos.api': ['/sdk/api/@ohos.api.d.ets'],
    '@arkts.api': ['/sdk/arkts/@arkts.api.d.ets'],
    '@kit.api': ['/sdk/kits/@kit.api.d.ets']
};

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

/**
 * Collects mock config info and asserts that the given configuration is rejected
 * with the mock-specific error code, the fixed short description, and a detailed
 * cause (the "Error Message" line) containing `causePart`.
 */
function expectMockConfigError(configJson5: string, causePart: string, overrides: Partial<BuildConfig> = {}): void {
    let thrown: unknown;
    try {
        new MockConfigGenerator(makeBuildConfig(configJson5, overrides), SYSTEM_API_SECTION).collectMockConfigInfo();
    } catch (error) {
        thrown = error;
    }
    expect(thrown).toBeInstanceOf(DriverError);
    const logData = (thrown as DriverError).logData;
    expect(logData.code).toBe('11410040');
    expect(logData.description).toBe('Incorrect mock configuration.');
    expect(logData.cause).toContain(causePart);
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
        const a = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        const b = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        expect(a).not.toBe(b);
        // mutating one must not affect the other
        expect(a.getMockConfigInfo()).not.toBe(b.getMockConfigInfo());
    });
});

describe('MockConfigGenerator mockParams getter', () => {
    test('exposes the buildConfig mockParams', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/ohos_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        expect(gen.mockParams.mockConfigPath).toBe(bc.mockParams!.mockConfigPath);
    });
});

describe('MockConfigGenerator collectMockConfigInfo - valid', () => {
    test('system API key (@ohos/@arkts/@kit) is kept as-is with an absolute mock source', () => {
        for (const prefix of ['@ohos', '@arkts', '@kit']) {
            const bc = makeBuildConfig(`{ "${prefix}.api": { source: "mocks/ohos_mock.ets" } }`);
            const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
            gen.collectMockConfigInfo();
            const info = gen.getMockConfigInfo();
            expect(info[`${prefix}.api`]).toBeDefined();
            expect(info[`${prefix}.api`].source).toBe(path.join(moduleRootPath, 'mocks', 'ohos_mock.ets'));
        }
    });

    test('relative key is resolved to an absolute key', () => {
        const bc = makeBuildConfig(`{ "./bar": { source: "mocks/rel_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'rel_mock.ets');
        const absKey = path.join(tmpDir, 'bar.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
    });

    test('source-file key (no prefix) resolves to src/main/ets', () => {
        const bc = makeBuildConfig(`{ "util": { source: "mocks/util_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        gen.collectMockConfigInfo();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'util_mock.ets');
        const absKey = path.join(moduleRootPath, 'src', 'main', 'ets', 'util.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
    });

    test('double-quoted "use static" first line is rejected (only single-quote \'use static\' accepted)', () => {
        expectMockConfigError(`{ "@ohos.foo": { source: "mocks/dq_mock.ets" } }`,
            `The mock file for "mocks/dq_mock.ets" is invalid`);
    });
});

describe('MockConfigGenerator collectMockConfigInfo - invalid', () => {
    test('entry missing a string source field reports an error', () => {
        expectMockConfigError(`{ "@ohos.foo": {} }`,
            'Each entry must be an object with a "source" string field');
    });

    test('mock source without a use-static first line reports an error', () => {
        expectMockConfigError(`{ "@ohos.foo": { source: "mocks/bad_mock.ets" } }`,
            `The mock file for "mocks/bad_mock.ets" is invalid`);
    });

    test('key file without a use-static first line reports an error', () => {
        writeUseStaticFile(path.join(moduleRootPath, 'src', 'main', 'ets', 'nostatic.ets'), NO_USE_STATIC);
        expectMockConfigError(`{ "nostatic": { source: "mocks/ohos_mock.ets" } }`,
            `The key file for "nostatic" is invalid`);
    });

    test('non-existent mock source reports an error', () => {
        expectMockConfigError(`{ "@ohos.foo": { source: "mocks/nonexistent.ets" } }`,
            'Source file not found under');
    });

    test('key that is not a system API / relative / dependency / source file reports an error', () => {
        expectMockConfigError(`{ "no_such_key": { source: "mocks/ohos_mock.ets" } }`,
            'Key must be a system API');
    });

    test('relative key whose target file does not exist reports an error', () => {
        expectMockConfigError(`{ "./missing": { source: "mocks/ohos_mock.ets" } }`,
            `The key file for "./missing" is invalid`);
    });

    test.each(['@ohos', '@arkts', '@kit'])('non-existent system API key (%s.nonexist) reports an error', (prefix) => {
        expectMockConfigError(`{ "${prefix}.nonexist": { source: "mocks/ohos_mock.ets" } }`,
            `System API "${prefix}.nonexist" does not exist`);
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
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        expect(() => gen.collectMockConfigInfo()).not.toThrow();
        const info = gen.getMockConfigInfo();
        expect(info['@lib/har']).toBeDefined();
        expect(info['@lib/har'].source).toBe(path.join(moduleRootPath, 'mocks', 'ohos_mock.ets'));
    });

    test('key "<pkg>/<sub>" resolves against the dependency module path', () => {
        const depPath = path.join(tmpDir, 'dep');
        writeUseStaticFile(path.join(depPath, 'sub.ets'));
        const bc = makeBuildConfig(`{ "@lib/har/sub": { source: "mocks/ohos_mock.ets" } }`, {
            dependencyModuleList: [makeDep(depPath)]
        });
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        expect(() => gen.collectMockConfigInfo()).not.toThrow();
        const info = gen.getMockConfigInfo();

        const expectedSource = path.join(moduleRootPath, 'mocks', 'ohos_mock.ets');
        // resolved dependency path, pointing to the mock source
        const absKey = path.join(depPath, 'sub.ets');
        expect(info[absKey]).toBeDefined();
        expect(info[absKey].source).toBe(expectedSource);
    });

    test('dependency sub-path that does not exist reports an error', () => {
        const depPath = path.join(tmpDir, 'dep');
        // dep/sub.ets intentionally NOT created
        expectMockConfigError(`{ "@lib/har/sub": { source: "mocks/ohos_mock.ets" } }`,
            'File not found under', {
                dependencyModuleList: [makeDep(depPath)]
            });
    });
});

describe('MockConfigGenerator getMockConfigInfo', () => {
    test('returns an empty map before collectMockConfigInfo and a populated map after', () => {
        const bc = makeBuildConfig(`{ "@ohos.foo": { source: "mocks/ohos_mock.ets" } }`);
        const gen = new MockConfigGenerator(bc, SYSTEM_API_SECTION);
        expect(Object.keys(gen.getMockConfigInfo())).toHaveLength(0);

        gen.collectMockConfigInfo();
        expect(Object.keys(gen.getMockConfigInfo())).toHaveLength(1);
    });
});
