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

// This file has to mock a lot because compile_worker.ts only runs a `process.on`.

jest.mock('fs');
jest.mock('path');
jest.mock('../../../src/utils', () => ({
    changeFileExtension: jest.fn((p, ext) => p.replace(/\.[^/.]+$/, ext)),
    ensurePathExists: jest.fn()
}));
jest.mock('../../../src/plugins/plugins_driver', () => {
    const mPluginDriver = {
        initPlugins: jest.fn(),
        getPluginContext: jest.fn(() => ({ setArkTSProgram: jest.fn() })),
        runPluginHook: jest.fn(),
        getInstance: jest.fn((): any => mPluginDriver)
    } as any;
    return { PluginDriver: mPluginDriver, PluginHook: { PARSED: 1, CHECKED: 2, CLEAN: 3 } };
});
jest.mock('../../../src/logger', () => {
    const mLogger = {
        printError: jest.fn(),
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
    KOALA_WRAPPER_PATH_FROM_SDK: 'koala'
}));
jest.mock('/sdk/koala', () => ({
    arkts: fakeArkts,
    arktsGlobal: fakeArktsGlobal
}), { virtual: true });

const fakeArkts = {
    Config: { create: jest.fn(() => ({ peer: 'peer' })) },
    Context: { createFromString: jest.fn(() => ({ program: {}, peer: 'peer' })) },
    proceedToState: jest.fn(),
    Es2pandaContextState: { ES2PANDA_STATE_PARSED: 1, ES2PANDA_STATE_CHECKED: 2, ES2PANDA_STATE_BIN_GENERATED: 3 },
    generateStaticDeclarationsFromContext: jest.fn(),
    destroyConfig: jest.fn()
};

const fakeArktsGlobal = {
    es2panda: {
        _SetUpSoPath: jest.fn(),
        _DestroyContext: jest.fn((pandaSDKPath: string) => {
            return;
        }),
    },
    filePath: '',
    config: '',
    compilerContext: { program: {}, peer: 'peer' }
};

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

jest.mock('path', () => ({
    ...jest.requireActual('path'),
    resolve: jest.fn((...args) => args.join('/')),
    relative: jest.fn((from, to) => to.replace(from, '')),
    join: jest.fn((...args) => args.join('/')),
    basename: jest.fn((p) => p.split('/').pop())
}));

jest.mock('module', () => ({
    createRequire: () => () => ({ arkts: fakeArkts, arktsGlobal: fakeArktsGlobal })
}));

beforeEach(() => {
    jest.resetModules();
    (process as any).send = jest.fn();
    jest.spyOn(process, 'exit').mockImplementation((() => { throw new Error('exit'); }) as any);
});
afterEach(() => {
    jest.clearAllMocks();
});

// Test the functions of the compile_worker.ts file
import { changeFileExtension } from '../../../src/utils';
import { DECL_ETS_SUFFIX } from '../../../src/pre_define';
describe('compile_worker', () => {
    const fileInfo = {
        filePath: '/src/foo.ets',
        abcFilePath: '/out/foo.abc',
        arktsConfigFile: '/src/arktsconfig.json'
    };
    const buildConfig = {
        buildMode: 0,
        hasMainModule: true,
        byteCodeHar: true,
        moduleType: 0,
        declgenV2OutPath: '/decl',
        packageName: 'pkg',
        moduleRootPath: '/src',
        buildSdkPath: '/sdk'
    };
    const moduleInfos: any[] = [];

    test('compile all files && exit(0)', () => {
        require('fs').readFileSync.mockReturnValue(Buffer.from('source code'));
        require('path').relative.mockImplementation((from: string, to: string) => to.replace(from, '').replace(/^\//, ''));
        require('path').join.mockImplementation((...args: string[]) => args.join('/'));
        require('path').resolve.mockImplementation((...args: string[]) => args.join('/'));
        require('path').basename.mockImplementation((p: string) => p.split('/').pop());
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).toThrow('exit');
        expect(require('../../../src/utils').ensurePathExists).toHaveBeenCalled();
        expect(require('fs').readFileSync).toHaveBeenCalledWith(fileInfo.filePath);
        expect(fakeArkts.Config.create).toHaveBeenCalled();
        expect(fakeArkts.Context.createFromString).toHaveBeenCalled();
        expect(fakeArkts.proceedToState).toHaveBeenCalledTimes(3);
        expect(fakeArkts.generateStaticDeclarationsFromContext).toHaveBeenCalled();
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
        expect(process.exit).toHaveBeenCalledWith(0);
    });

    test('handle error && send fail message', () => {
        jest.spyOn(process, 'exit').mockImplementation(() => undefined as never);

        require('fs').readFileSync.mockImplementation(() => { throw new Error('fail'); });
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).not.toThrow();

        expect(process.send).toHaveBeenCalledWith(expect.objectContaining({
            success: false,
            filePath: fileInfo.filePath,
            error: expect.any(String)
        }));
    });

    test('generate decl file', () => {
        require('fs').readFileSync.mockReturnValue(Buffer.from('source code'));
        let config = { ...buildConfig, hasMainModule: false };
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig: config, moduleInfos });
        }).toThrow('exit');
        expect(fakeArkts.generateStaticDeclarationsFromContext).not.toHaveBeenCalled();
        require('fs').readFileSync.mockReturnValue(Buffer.from('source code'));
        config = { ...buildConfig, byteCodeHar: false, moduleType: 9999 };
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig: config, moduleInfos });
        }).toThrow('exit');
        expect(fakeArkts.generateStaticDeclarationsFromContext).not.toHaveBeenCalled();
        require('path').relative.mockImplementation((from: string, to: string) => to.replace(from, '').replace(/^\//, ''));
        require('fs').readFileSync.mockReturnValue(Buffer.from('source code'));
        config = { ...buildConfig, hasMainModule: true, byteCodeHar: true };
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig: config, moduleInfos });
        }).toThrow('exit');
        let filePathFromModuleRoot = require('path').relative(buildConfig.moduleRootPath, fileInfo.filePath);
        let declarationPath = require('path').join(buildConfig.declgenV2OutPath, filePathFromModuleRoot);
        let declarationFilePath = changeFileExtension(declarationPath, DECL_ETS_SUFFIX);
        expect(fakeArkts.generateStaticDeclarationsFromContext).toHaveBeenCalledWith(declarationFilePath);
    });

    test('throw while process.send is undefined', () => {
        delete (process as any).send;
        require('../../../src/build/compile_worker');
        expect(() => {
            (process as any).emit('message', { taskList: [fileInfo], buildConfig, moduleInfos });
        }).toThrow('process.send is undefined. This worker must be run as a forked process.');
    });
});
