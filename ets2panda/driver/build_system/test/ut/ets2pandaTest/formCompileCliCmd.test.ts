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

import * as path from 'path';
import type { BuildConfig, CompileJobInfo } from '../../../src/types';
import { BUILD_TYPE, CompileJobType, JobContentType } from '../../../src/types';
import { Ets2panda } from '../../../src/util/ets2panda';
import { MERGED_INTERMEDIATE_FILE, SYMBOL_TABLE_FILE } from '../../../src/pre_define';

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn(() => ({}))
}));

jest.mock('../../../src/logger', () => {
    const actual = jest.requireActual('../../../src/logger');
    return {
        Logger: {
            getInstance: jest.fn(() => ({
                printDebug: jest.fn(),
                printInfo: jest.fn(),
                printWarn: jest.fn(),
                printError: jest.fn(),
                printErrorAndExit: jest.fn()
            })),
            destroyInstance: jest.fn()
        },
        LogDataFactory: actual.LogDataFactory
    };
});

jest.mock('../../../src/plugins/plugins_driver', () => ({
    PluginDriver: {
        getInstance: jest.fn(() => ({
            initPlugins: jest.fn(),
            getPluginContext: jest.fn(() => ({})),
            runPluginHook: jest.fn()
        }))
    }
}));

function createBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
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
        plugins: {},
        paths: {},
        buildType: BUILD_TYPE.BUILD,
        ...overrides
    } as BuildConfig;
}

function createClusterJob(): CompileJobInfo {
    return {
        contentType: JobContentType.CLUSTER,
        content: [{ input: '/mock/src/a.ets', output: '' }],
        arktsConfig: '/mock/cache/testPackage/arktsconfig.json',
        moduleName: 'testPackage',
        moduleRoot: '/mock/module',
        declgenConfig: { output: '/mock/decl' },
        jobType: CompileJobType.ABC
    };
}

function formCmd(buildConfig: BuildConfig): string[] {
    Ets2panda.getInstance(buildConfig);
    const cmd = (Ets2panda.getInstance() as any).formCompileCliCmd(createClusterJob(), false);
    Ets2panda.destroyInstance();
    return cmd;
}

describe('Ets2panda formCompileCliCmd reload flags', () => {
    afterEach(() => {
        Ets2panda.destroyInstance();
    });

    test('non-reload build passes no reload flags', () => {
        const cmd = formCmd(createBuildConfig());
        expect(cmd.some((arg: string) => arg.startsWith('--dump-symbol-table'))).toBe(false);
        expect(cmd.some((arg: string) => arg.startsWith('--input-symbol-table'))).toBe(false);
        expect(cmd).not.toContain('--hot-reload');
        expect(cmd).not.toContain('--cold-reload');
        // output stays in the cache dir
        expect(cmd).toContain(path.resolve('/mock/cache', MERGED_INTERMEDIATE_FILE));
    });

    test('full-build reload dumps the symbol table into the reload intermediate dir', () => {
        const cmd = formCmd(createBuildConfig({
            buildType: BUILD_TYPE.HOT_RELOAD,
            reload: { isFullBuild: true, patchAbcPath: '/mock/reload_out' }
        }));
        expect(cmd).toContain(`--dump-symbol-table=${path.resolve('/mock/cache/reload', SYMBOL_TABLE_FILE)}`);
        expect(cmd.some((arg: string) => arg.startsWith('--input-symbol-table'))).toBe(false);
        expect(cmd).not.toContain('--hot-reload');
        expect(cmd).not.toContain('--cold-reload');
        // full build keeps its output in the main cache dir
        expect(cmd).toContain(path.resolve('/mock/cache', MERGED_INTERMEDIATE_FILE));
    });

    test('incremental hot reload passes --hot-reload and the input symbol table', () => {
        const cmd = formCmd(createBuildConfig({
            buildType: BUILD_TYPE.HOT_RELOAD,
            reload: { isFullBuild: false, patchAbcPath: '/mock/reload_out', changedFileList: '/mock/list.json5' }
        }));
        expect(cmd).toContain('--hot-reload');
        expect(cmd).not.toContain('--cold-reload');
        expect(cmd).toContain(`--input-symbol-table=${path.resolve('/mock/cache/reload', SYMBOL_TABLE_FILE)}`);
        expect(cmd.some((arg: string) => arg.startsWith('--dump-symbol-table'))).toBe(false);
        // incremental reload keeps intermediates in <cachePath>/reload/
        expect(cmd).toContain(path.resolve('/mock/cache/reload', MERGED_INTERMEDIATE_FILE));
        // nothing but the arklink result may land in patchAbcPath
        expect(cmd.some((arg: string) => arg.includes('/mock/reload_out'))).toBe(false);
    });

    test('incremental cold reload passes --cold-reload', () => {
        const cmd = formCmd(createBuildConfig({
            buildType: BUILD_TYPE.COLD_RELOAD,
            reload: { isFullBuild: false, patchAbcPath: '/mock/reload_out', changedFileList: '/mock/list.json5' }
        }));
        expect(cmd).toContain('--cold-reload');
        expect(cmd).not.toContain('--hot-reload');
        expect(cmd).toContain(`--input-symbol-table=${path.resolve('/mock/cache/reload', SYMBOL_TABLE_FILE)}`);
    });
});
