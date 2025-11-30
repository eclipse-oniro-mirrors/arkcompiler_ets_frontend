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

// This file has to mock a lot because declgen_worker.ts only runs a `process.on`.

jest.mock('fs');
jest.mock('path');
jest.mock('../../../src/util/utils', () => ({
    // simplified functions for testing
    changeFileExtension: jest.fn((file: string, targetExt: string, originExt = '') => {
        const currentExt = originExt.length === 0 ? file.substring(file.lastIndexOf('.'))
            : originExt;
        const fileWithoutExt = file.substring(0, file.lastIndexOf(currentExt));
        return fileWithoutExt + targetExt;
    }),
    changeDeclgenFileExtension: jest.fn((file: string, targetExt: string) => {
        const DECL_ETS_SUFFIX = '.d.ets';
        if (file.endsWith(DECL_ETS_SUFFIX)) {
            return file.replace(DECL_ETS_SUFFIX, targetExt);
        }
        return file.substring(0, file.lastIndexOf('.')) + targetExt;
    }),
    createFileIfNotExists: jest.fn(),
    ensurePathExists: jest.fn()
}));
jest.mock('../../../src/plugins/plugins_driver', () => {
    const mPluginDriver = {
        initPlugins: jest.fn(),
        getPluginContext: jest.fn(() => ({
            setArkTSProgram: jest.fn(),
            setArkTSAst: jest.fn()
        })),
        runPluginHook: jest.fn(),
        getInstance: jest.fn((): any => mPluginDriver)
    } as any;
    return { PluginDriver: mPluginDriver, PluginHook: { PARSED: 1, CHECKED: 2 } };
});

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn((buildConfig) => {
        const fakeKoala = {
            arkts: fakeArkts,
            arktsGlobal: fakeArktsGlobal
        };
        fakeKoala.arktsGlobal.es2panda._SetUpSoPath(buildConfig.pandaSdkPath);

        buildConfig.arkts = fakeKoala.arkts;
        buildConfig.arktsGlobal = fakeKoala.arktsGlobal;
        return fakeKoala;
    })
}));

jest.mock('../../../src/logger', () => {
    const mLogger = {
        printError: jest.fn(),
        printInfo: jest.fn(),
        getInstance: jest.fn((): any => mLogger)
    } as any;
    return {
        Logger: mLogger,
        LogDataFactory: {
            newInstance: jest.fn(() => ({
                code: '001', description: '', cause: '', position: '', solutions: [], moreInfo: {}
            }))
        }
    };
});
jest.mock('../../../src/pre_define', () => ({
    DECL_ETS_SUFFIX: '.d.ets',
    TS_SUFFIX: '.ts',
    KOALA_WRAPPER_PATH_FROM_SDK: 'koala'
}));

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn((buildConfig) => {
        const fakeKoala = {
            arkts: fakeArkts,
            arktsGlobal: fakeArktsGlobal
        };
        fakeKoala.arktsGlobal.es2panda._SetUpSoPath(buildConfig.pandaSdkPath);

        buildConfig.arkts = fakeKoala.arkts;
        buildConfig.arktsGlobal = fakeKoala.arktsGlobal;
        return fakeKoala;
    })
}));

const fakeArkts = {
    Config: { create: jest.fn(() => ({ peer: 'peer' })) },
    Context: {
        createFromString: jest.fn(() => ({ program: {}, peer: 'peer' })),
        createFromStringWithHistory: jest.fn(() => ({ program: {}, peer: 'peer' }))
    },
    proceedToState: jest.fn(),
    Es2pandaContextState: { ES2PANDA_STATE_PARSED: 1, ES2PANDA_STATE_CHECKED: 2 },
    generateTsDeclarationsFromContext: jest.fn(),
    destroyConfig: jest.fn(),
    EtsScript: { fromContext: jest.fn(() => ({})) }
};
const fakeArktsGlobal = {
    es2panda: {
        _SetUpSoPath: jest.fn(),
        _DestroyContext: jest.fn()
    },
    filePath: '',
    config: '',
    compilerContext: { program: {}, peer: 'peer' }
};

jest.mock('/sdk/koala', () => ({
    arkts: fakeArkts,
    arktsGlobal: fakeArktsGlobal,
}), { virtual: true });

beforeEach(() => {
    jest.resetModules();
    (process as any).send = jest.fn();
    jest.spyOn(process, 'exit').mockImplementation((() => { throw new Error('exit'); }) as any);
    require('path').resolve.mockImplementation((...args: string[]) => {
        if (args.includes('koala')) return '/sdk/koala';
        return args.join('/');
    });
    require('path').join.mockImplementation((...args: string[]) => args.join('/'));
    require('path').relative.mockImplementation((from: string, to: string) => to.replace(from, '').replace(/^\//, ''));
    require('path').basename.mockImplementation((p: string) => p.split('/').pop());
});
afterEach(() => {
    jest.clearAllMocks();
});

// Test the functions of the declgen_worker.ts file
describe('declgen_worker', () => {
    const compileFileInfo = {
        filePath: '/src/foo.ets',
        dependentFiles: [],
        abcFilePath: 'foo.abc',
        arktsConfigFile: '/src/arktsconfig.json',
        packageName: 'pkg',
    };

    const buildConfig = {
        hasMainModule: true,
        byteCodeHar: true,
        moduleType: 9999,
        declgenV2OutPath: 'declgenV2Out',
        moduleRootPath: '/src',
        plugins: { pkg: 'plugin' },
        paths: { pkg: ['plugin'] },
        compileFiles: ['/src/foo.ets'],
        entryFiles: ['/src/foo.ets'],
        dependentModuleList: [],
        aliasConfig: {},
    };

    const moduleInfo = {
        isMainModule: true,
        packageName: 'pkg',
        moduleRootPath: '/src',
        moduleType: 'type',
        sourceRoots: [],
        entryFile: 'foo.ets',
        arktsConfigFile: 'arktsconfig.json',
        compileFileInfos: [],
        declgenV1OutPath: undefined,
        declgenV2OutPath: undefined,
        declgenBridgeCodePath: undefined,
        byteCodeHar: false,
        staticDepModuleInfos: new Map(),
        dynamicDepModuleInfos: new Map(),
        dependenciesSet: new Set(),
        dependentSet: new Set(),
    };

    const moduleInfos = [['pkg', moduleInfo]];

    test('generate declaration && glue files && exit', () => {
        require('fs').readFileSync.mockReturnValue('source code');
        const id = 'processId1';
        const payload = {
            fileInfo: compileFileInfo,
            buildConfig: buildConfig,
            moduleInfos: moduleInfos,
        };
        require('../../../src/build/declgen_worker');
        (process as any).emit('message', { id, payload });

        expect(require('../../../src/util/utils').ensurePathExists).toHaveBeenCalledTimes(2);
        expect(fakeArkts.Config.create).toHaveBeenCalled();
        expect(fakeArkts.Context.createFromStringWithHistory).toHaveBeenCalled();
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(1, 'peer', true);
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(2, 'peer', true);
        expect(fakeArkts.EtsScript.fromContext).toHaveBeenCalled();
        expect(fakeArkts.generateTsDeclarationsFromContext).toHaveBeenCalled();
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
        expect(process.send).toHaveBeenCalledWith({
            id: 'processId1',
            success: true,
            shouldKill: false
        });
    });

    test('destroy context && config', () => {
        require('fs').readFileSync.mockReturnValue('source code');
        require('../../../src/build/declgen_worker');
        const processId = 'processId3';
        const payload = {
            fileInfo: compileFileInfo,
            buildConfig: buildConfig,
            moduleInfos: moduleInfos,
        };
        (process as any).emit('message', { processId, payload });
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
    });

});
