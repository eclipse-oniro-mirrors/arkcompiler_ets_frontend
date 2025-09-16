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
jest.mock('../../../src/utils', () => ({
    changeFileExtension: jest.fn((p: string, ext: string) => p.replace(/\.[^/.]+$/, ext)),
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
jest.mock('../../../src/logger', () => {
    const mLogger = {
        printError: jest.fn(),
        printInfo: jest.fn(),
        getInstance: jest.fn((): any => mLogger)
    } as any;
    return {
        Logger: mLogger,
        LogDataFactory: { newInstance: jest.fn(() => ({
            code: '001', description: '', cause: '', position: '', solutions: [], moreInfo: {} })) }
    };
});
jest.mock('../../../src/pre_define', () => ({
    DECL_ETS_SUFFIX: '.d.ets',
    TS_SUFFIX: '.ts',
    KOALA_WRAPPER_PATH_FROM_SDK: 'koala'
}));

const fakeArkts = {
    Config: { create: jest.fn(() => ({ peer: 'peer' })) },
    Context: { createFromString: jest.fn(() => ({ program: {}, peer: 'peer' })) },
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
    arktsGlobal: fakeArktsGlobal
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
    const fileInfo = {
        filePath: '/src/foo.ets',
        arktsConfigFile: '/src/arktsconfig.json',
        packageName: 'pkg'
    };
    const buildConfig = {
        buildSdkPath: '/sdk',
        pandaSdkPath: '/panda'
    };
    const moduleInfo = {
        moduleRootPath: '/src',
        declgenV1OutPath: '/decl',
        declgenBridgeCodePath: '/bridge',
        packageName: 'pkg'
    };
    const moduleInfos = [['pkg', moduleInfo]];

    test('generate declaration && glue files && exit', () => {
        require('fs').readFileSync.mockReturnValue('source code');
        require('../../../src/build/declgen_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).toThrow('exit');

        expect(require('../../../src/utils').ensurePathExists).toHaveBeenCalledTimes(2);
        expect(fakeArkts.Config.create).toHaveBeenCalled();
        expect(fakeArkts.Context.createFromString).toHaveBeenCalled();
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(1, 'peer', true);
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(2, 'peer', true);
        expect(fakeArkts.EtsScript.fromContext).toHaveBeenCalled();
        expect(fakeArkts.generateTsDeclarationsFromContext).toHaveBeenCalled();
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
        expect(process.exit).toHaveBeenCalledWith(0);
        expect(process.send).toHaveBeenCalledWith(expect.objectContaining({
            success: true,
            filePath: fileInfo.filePath
        }));
    });

    test('handle error && send fail message', () => {
        jest.spyOn(process, 'exit').mockImplementation(() => undefined as never);
        require('fs').readFileSync.mockImplementation(() => { throw new Error('fail'); });
        require('../../../src/build/declgen_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).not.toThrow();

        expect(process.send).toHaveBeenCalledWith(expect.objectContaining({
            success: false,
            filePath: fileInfo.filePath,
            error: expect.any(String)
        }));
    });

    test('throw if process.send is undefined', () => {
        delete (process as any).send;
        require('../../../src/build/declgen_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).toThrow('process.send is undefined. This worker must be run as a forked process.');
    });

    test('destroy context && config', () => {
        require('fs').readFileSync.mockReturnValue('source code');
        require('../../../src/build/declgen_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).toThrow('exit');
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
    });
});
