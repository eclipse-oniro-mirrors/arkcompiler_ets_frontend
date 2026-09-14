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
import * as os from 'os';
import * as fs from 'fs';
import type { BuildConfig, ReloadConfig } from '../../../src/types';
import { BUILD_TYPE, ES2PANDA_MODE } from '../../../src/types';
import { BaseMode } from '../../../src/build/base_mode';
import { BuildMode } from '../../../src/build/build_mode';
import { RecordEvent } from '../../../src/util/statsRecorder';
import {
    ARKTSCONFIG_JSON_FILE,
    LINKER_INPUT_FILE,
    MERGED_ABC_FILE,
    MERGED_INTERMEDIATE_FILE,
    SYMBOL_TABLE_FILE
} from '../../../src/pre_define';

jest.mock('fs');
jest.mock('../../../src/logger', () => {
    const actual = jest.requireActual('../../../src/logger');
    const loggerInstance = {
        printDebug: jest.fn(),
        printInfo: jest.fn(),
        printWarn: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn()
    };
    return {
        Logger: {
            getInstance: jest.fn(() => loggerInstance)
        },
        LogDataFactory: actual.LogDataFactory
    };
});

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn(() => ({
        arkts: {},
        arktsGlobal: {}
    }))
}));

jest.mock('../../../src/util/utils', () => ({
    ...jest.requireActual('../../../src/util/utils'),
    ensurePathExists: jest.fn(),
    safeRealpath: jest.fn((p: string) => p)
}));

const mockEts = {
    initalize: jest.fn(),
    compile: jest.fn(),
    finalize: jest.fn()
};
jest.mock('../../../src/util/ets2panda', () => ({
    Ets2panda: {
        getInstance: jest.fn(() => mockEts),
        destroyInstance: jest.fn()
    }
}));

class TestBaseMode extends BaseMode {
    constructor(buildConfig: BuildConfig) {
        super(buildConfig);
    }
    public runReadChangeFileList(): Set<string> { return (this as any).readChangeFileList(); }
    public runMergeAbcFilesForReload(intermediateFile: string): void {
        (this as any).mergeAbcFilesForReload(intermediateFile);
    }
    public callAbcLinker(allFiles: string[], mergedFile: string, workDir: string, stripUnused: boolean): void {
        (this as any).runAbcLinker(allFiles, mergedFile, workDir, stripUnused);
    }
}

function createReloadConfig(overrides: Partial<ReloadConfig> = {}): ReloadConfig {
    return {
        isFullBuild: false,
        changedFileList: '/mock/changefilelist_static.json5',
        patchAbcPath: '/mock/reload_out',
        ...overrides
    };
}

function createReloadBuildConfig(overrides: Partial<BuildConfig> = {}): BuildConfig {
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
        buildType: BUILD_TYPE.HOT_RELOAD,
        reload: createReloadConfig(),
        abcLinkerPath: '/mock/abc_linker',
        ...overrides
    } as BuildConfig;
}

describe('BaseMode reload mode', () => {
    beforeEach(() => {
        jest.clearAllMocks();
        // restoreAllMocks only undoes jest.spyOn spies; implementations set on the
        // shared plain jest.fn()s (e.g. the DriverError-throwing compile) survive
        // clearAllMocks and must be reset explicitly
        mockEts.compile.mockReset();
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('[]');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.readdirSync as jest.Mock).mockReturnValue([]);
        (fs.statSync as jest.Mock).mockReturnValue({
            mtimeMs: 0,
            isDirectory: () => false,
            isFile: () => true
        });
    });

    afterEach(() => {
        jest.restoreAllMocks();
    });

    test('isReloadMode is true only for hotreload/coldreload', () => {
        const hotMode = new TestBaseMode(createReloadBuildConfig());
        const coldMode = new TestBaseMode(createReloadBuildConfig({ buildType: BUILD_TYPE.COLD_RELOAD }));
        const buildMode = new TestBaseMode(createReloadBuildConfig({ buildType: BUILD_TYPE.BUILD }));
        expect(hotMode.isReloadMode).toBe(true);
        expect(coldMode.isReloadMode).toBe(true);
        expect(buildMode.isReloadMode).toBe(false);
    });

    test('isReloadMode is false for a full-build reload (first invocation)', () => {
        const fullReload = createReloadConfig({ isFullBuild: true, changedFileList: undefined });
        const hotFull = new TestBaseMode(createReloadBuildConfig({ reload: fullReload }));
        const coldFull = new TestBaseMode(
            createReloadBuildConfig({ buildType: BUILD_TYPE.COLD_RELOAD, reload: fullReload })
        );
        expect(hotFull.isReloadMode).toBe(false);
        expect(coldFull.isReloadMode).toBe(false);
    });

    test('reload constructor skips processBuildConfig (moduleInfos stays empty)', () => {
        const mode = new TestBaseMode(createReloadBuildConfig({ compileFiles: ['/mock/src/a.ets'] }));
        expect((mode as any).moduleInfos.size).toBe(0);
        expect((mode as any).fileToModule.size).toBe(0);
        // entryFiles keeps the compileFiles seed: no filtering/replacement happened
        expect((mode as any).entryFiles).toEqual(new Set(['/mock/src/a.ets']));
    });

    test('full-build reload constructor runs processBuildConfig (moduleInfos populated)', () => {
        const reload = createReloadConfig({ isFullBuild: true, changedFileList: undefined });
        // the file must live under the main module root, otherwise processEntryFiles rejects it
        const mode = new TestBaseMode(createReloadBuildConfig({
            reload,
            compileFiles: ['/mock/module/src/a.ets']
        }));
        expect((mode as any).moduleInfos.size).toBeGreaterThan(0);
        expect((mode as any).moduleInfos.has('testPackage')).toBe(true);
        expect((mode as any).fileToModule.has(path.resolve('/mock/module/src/a.ets'))).toBe(true);
    });

    describe('readChangeFileList', () => {
        test('returns empty set when the reload field is missing', () => {
            const mode = new TestBaseMode(createReloadBuildConfig({ reload: undefined }));
            expect(mode.runReadChangeFileList().size).toBe(0);
        });

        test('returns empty set when changedFileList is absent (full-build reload)', () => {
            const reload = createReloadConfig({ isFullBuild: true, changedFileList: undefined });
            const mode = new TestBaseMode(createReloadBuildConfig({ reload }));
            expect(mode.runReadChangeFileList().size).toBe(0);
        });

        test('returns empty set when the file does not exist', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(false);
            const mode = new TestBaseMode(createReloadBuildConfig());
            expect(mode.runReadChangeFileList().size).toBe(0);
        });

        test('reads filePath entries from modifiedStaticFiles (JSON5, other fields ignored)', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(
                `{\n  // changed files\n  "modifiedStaticFiles": [\n    { "filePath": '/mock/src/a.ets' },\n    { "filePath": '/mock/src/b.ets' },\n  ],\n  "modifiedDynamicFiles": [{ "filePath": '/mock/dyn/a.ets' }],\n}`
            );
            const mode = new TestBaseMode(createReloadBuildConfig());
            expect(mode.runReadChangeFileList()).toEqual(new Set(['/mock/src/a.ets', '/mock/src/b.ets']));
        });

        test('returns empty set when modifiedStaticFiles is absent or empty', () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(`{ "modifiedDynamicFiles": [{ "filePath": '/mock/dyn/a.ets' }] }`);
            const modeWithoutField = new TestBaseMode(createReloadBuildConfig());
            expect(modeWithoutField.runReadChangeFileList().size).toBe(0);

            (fs.readFileSync as jest.Mock).mockReturnValue(`{ "modifiedStaticFiles": [] }`);
            const modeWithEmpty = new TestBaseMode(createReloadBuildConfig());
            expect(modeWithEmpty.runReadChangeFileList().size).toBe(0);
        });
    });

    describe('runSimultaneousForReload', () => {
        test('skips silently when there are no changed files', async () => {
            const mode = new TestBaseMode(createReloadBuildConfig({ reload: undefined }));
            await mode.runSimultaneousForReload();

            const logger = require('../../../src/logger').Logger.getInstance();
            expect(logger.printInfo).toHaveBeenCalledWith('No changed files for reload, skip.');
            expect(require('../../../src/util/ets2panda').Ets2panda.getInstance).not.toHaveBeenCalled();
            expect(mockEts.compile).not.toHaveBeenCalled();
        });

        test('compiles changed files with reused arktsconfig and merges into reloadOutPath', async () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(
                `{ "modifiedStaticFiles": [{ "filePath": '/mock/src/a.ets' }, { "filePath": '/mock/src/b.ets' }] }`
            );
            const child_process = require('child_process');
            const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));

            const mode = new TestBaseMode(createReloadBuildConfig());
            await mode.runSimultaneousForReload();

            // compile job: one cluster of the changed files, reusing the prior arktsconfig
            expect(mockEts.compile).toHaveBeenCalledTimes(1);
            const [jobId, job, incremental] = mockEts.compile.mock.calls[0];
            expect(jobId).toBe('ReloadBuildId');
            expect(incremental).toBe(false);
            expect(job.arktsConfig).toBe(path.resolve('/mock/cache', 'testPackage', ARKTSCONFIG_JSON_FILE));
            expect(job.moduleName).toBe('testPackage');
            expect(job.moduleRoot).toBe('/mock/module');
            expect(job.content).toEqual([
                { input: '/mock/src/a.ets', output: '' },
                { input: '/mock/src/b.ets', output: '' }
            ]);
            expect(require('../../../src/util/ets2panda').Ets2panda.destroyInstance).toHaveBeenCalled();

            // merge: linker input and intermediate abc live in the reload intermediate
            // dir under cachePath, while the merged abc goes into reloadOutPath
            const reloadOut = '/mock/reload_out';
            const intermediateDir = path.resolve('/mock/cache', 'reload');
            const intermediate = path.resolve(intermediateDir, MERGED_INTERMEDIATE_FILE);
            expect(fs.writeFileSync).toHaveBeenCalledWith(path.join(intermediateDir, LINKER_INPUT_FILE), intermediate);
            const execArg = execSpy.mock.calls[0][0] as string;
            expect(execArg).toContain('--output');
            expect(execArg).toContain('"' + path.resolve(reloadOut, MERGED_ABC_FILE) + '"');
            expect(execArg).toContain('@"' + path.join(intermediateDir, LINKER_INPUT_FILE) + '"');
            // the reload patch keeps every symbol referenced by unchanged code
            expect(execArg).not.toContain('--strip-unused');
            // reloadOutPath holds only the arklink result: no intermediates leak into it
            expect(execArg).not.toContain(`/${reloadOut}/${MERGED_INTERMEDIATE_FILE}`);
            expect(execArg).not.toContain(`/${reloadOut}/${LINKER_INPUT_FILE}`);
            // the main merged abc must not be touched
            expect(execArg).not.toContain('/mock/output');

            execSpy.mockRestore();
        });

        test('throws when compile fails', async () => {
            (fs.existsSync as jest.Mock).mockReturnValue(true);
            (fs.readFileSync as jest.Mock).mockReturnValue(
                `{ "modifiedStaticFiles": [{ "filePath": '/mock/src/a.ets' }] }`
            );
            mockEts.compile.mockImplementation(() => {
                const { DriverError } = require('../../../src/util/error');
                const { LogDataFactory } = require('../../../src/logger');
                const logger = require('../../../src/logger').Logger.getInstance();
                jest.spyOn(logger, 'printError').mockImplementation(() => { });
                throw new DriverError(LogDataFactory.newInstance(10000000, 'fail'));
            });

            const mode = new TestBaseMode(createReloadBuildConfig());
            await expect(mode.runSimultaneousForReload()).rejects.toThrow('Reload build failed.');
        });
    });

    describe('mergeAbcFilesForReload / runAbcLinker', () => {
        test('reload merge skips bytecode-har collection and --strip-unused', () => {
            const child_process = require('child_process');
            const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));
            const mode = new TestBaseMode(createReloadBuildConfig());
            const collectSpy = jest.spyOn(mode as any, 'collectAbcFileFromByteCodeHar');

            mode.runMergeAbcFilesForReload(path.resolve('/mock/cache/reload', MERGED_INTERMEDIATE_FILE));

            expect(collectSpy).not.toHaveBeenCalled();
            expect(execSpy).toHaveBeenCalled();
            const execArg = execSpy.mock.calls[0][0] as string;
            expect(execArg).not.toContain('--strip-unused');
            execSpy.mockRestore();
        });

        test('regular merge still collects bytecode-har abc files and keeps --strip-unused', () => {
            const child_process = require('child_process');
            const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));
            const mode = new TestBaseMode(createReloadBuildConfig({ buildType: BUILD_TYPE.BUILD }));
            const collectSpy = jest.spyOn(mode as any, 'collectAbcFileFromByteCodeHar');

            (mode as any).mergeAbcFiles(['/mock/out.abc']);

            expect(collectSpy).toHaveBeenCalled();
            const execArg = execSpy.mock.calls[0][0] as string;
            expect(execArg).toContain('--strip-unused');
            execSpy.mockRestore();
        });

        test('runAbcLinker is a no-op for an empty file list', () => {
            const child_process = require('child_process');
            const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));
            const mode = new TestBaseMode(createReloadBuildConfig());

            mode.callAbcLinker([], path.resolve('/mock/reload_out', MERGED_ABC_FILE), '/mock/reload_out', true);

            expect(fs.writeFileSync).not.toHaveBeenCalled();
            expect(execSpy).not.toHaveBeenCalled();
            execSpy.mockRestore();
        });
    });
});

describe('BuildMode runReload', () => {
    beforeEach(() => {
        jest.clearAllMocks();
        // see the comment in the BaseMode describe above
        mockEts.compile.mockReset();
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('[]');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.readdirSync as jest.Mock).mockReturnValue([]);
        (fs.statSync as jest.Mock).mockReturnValue({
            mtimeMs: 0,
            isDirectory: () => false,
            isFile: () => true
        });
    });

    afterEach(() => {
        jest.restoreAllMocks();
    });

    test('runReload delegates to runSimultaneousForReload', async () => {
        const buildMode = new BuildMode(createReloadBuildConfig());
        // runReload invokes super.runSimultaneousForReload(), which resolves on
        // BaseMode.prototype — an instance-level spy would not intercept it
        const spy = jest.spyOn(BaseMode.prototype, 'runSimultaneousForReload')
            .mockResolvedValue(undefined);
        await buildMode.runReload();
        expect(spy).toHaveBeenCalledTimes(1);
        spy.mockRestore();
    });

    test('full-build reload compiles via runSimultaneous regardless of es2pandaMode', async () => {
        const reload = createReloadConfig({ isFullBuild: true, changedFileList: undefined });
        const buildMode = new BuildMode(createReloadBuildConfig({
            reload,
            es2pandaMode: ES2PANDA_MODE.RUN_PARALLEL
        }));

        const order: string[] = [];
        const ensurePathExists = require('../../../src/util/utils').ensurePathExists as jest.Mock;
        ensurePathExists.mockImplementation(() => { order.push('ensurePathExists'); });
        const runSpy = jest.spyOn(BuildMode.prototype, 'run')
            .mockImplementation(async () => { order.push('run'); });
        const simSpy = jest.spyOn(BaseMode.prototype, 'runSimultaneous')
            .mockImplementation(async () => { order.push('runSimultaneous'); });
        const reloadSpy = jest.spyOn(BaseMode.prototype, 'runSimultaneousForReload')
            .mockResolvedValue(undefined);

        await buildMode.runReload();

        // the symbol-table parent dir (the reload intermediate dir) is created
        // before the full build dumps into it
        expect(ensurePathExists).toHaveBeenCalledWith(path.join('/mock/cache/reload', SYMBOL_TABLE_FILE));
        // compile is pinned to simultaneous mode: the es2pandaMode dispatch inside run() is bypassed
        expect(simSpy).toHaveBeenCalledTimes(1);
        expect(runSpy).not.toHaveBeenCalled();
        expect(reloadSpy).not.toHaveBeenCalled();
        expect(order).toEqual(['ensurePathExists', 'runSimultaneous']);
        runSpy.mockRestore();
        simSpy.mockRestore();
        reloadSpy.mockRestore();
    });

    test('full-build reload runs the real simultaneous pipeline into the main output', async () => {
        const reload = createReloadConfig({ isFullBuild: true, changedFileList: undefined });
        const buildMode = new BuildMode(createReloadBuildConfig({
            reload,
            // even with a parallel es2pandaMode the real pipeline stays simultaneous
            es2pandaMode: ES2PANDA_MODE.RUN_PARALLEL,
            // the file must live under the main module root for processEntryFiles
            compileFiles: ['/mock/module/src/a.ets']
        }));

        const child_process = require('child_process');
        const execSpy = jest.spyOn(child_process, 'execSync').mockImplementation(() => Buffer.from(''));
        const statsRecordSpy = jest.spyOn((buildMode as any).statsRecorder, 'record');
        const writeSumSpy = jest.spyOn((buildMode as any).statsRecorder, 'writeSumSingle');

        await buildMode.runReload();

        // compile goes through the real runSimultaneous, reusing the cached arktsconfig
        expect(mockEts.compile).toHaveBeenCalledTimes(1);
        const [jobId, job, incremental] = mockEts.compile.mock.calls[0];
        expect(jobId).toBe('SimultaneousBuildId');
        expect(incremental).toBe(false);
        expect(job.arktsConfig).toBe(path.resolve('/mock/cache', 'testPackage', ARKTSCONFIG_JSON_FILE));
        expect(job.moduleName).toBe('testPackage');
        expect(job.content).toEqual([{ input: '/mock/module/src/a.ets', output: '' }]);

        // merge lands in the MAIN output with --strip-unused kept (full build, not a patch)
        const execArg = execSpy.mock.calls[0][0] as string;
        expect(execArg).toContain('--strip-unused');
        expect(execArg).toContain('"' + path.resolve('/mock/output', MERGED_ABC_FILE) + '"');
        expect(execArg).toContain('@"' + path.join('/mock/cache', LINKER_INPUT_FILE) + '"');
        expect(fs.writeFileSync).toHaveBeenCalledWith(
            path.join('/mock/cache', LINKER_INPUT_FILE),
            path.resolve('/mock/cache', MERGED_INTERMEDIATE_FILE)
        );
        // the patch dir stays untouched by the full build
        expect(execArg).not.toContain('/mock/reload_out');

        // stats recording matches what run() used to do
        expect(statsRecordSpy.mock.calls.some((args) => args[0] === RecordEvent.END)).toBe(true);
        expect(writeSumSpy).toHaveBeenCalledTimes(1);

        execSpy.mockRestore();
    });
});
