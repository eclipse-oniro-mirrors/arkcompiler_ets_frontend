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

// This file has to mock a lot because compile_thread_worker.ts only runs a `process.on`.

import { EventEmitter } from 'events';

jest.mock('fs');
jest.mock('path');
jest.mock('../../../src/util/utils', () => ({
    changeFileExtension: jest.fn((p: string, ext: string) => p.replace(/\.[^/.]+$/, ext)),
    ensurePathExists: jest.fn()
}));
jest.mock('../../../src/plugins/plugins_driver', () => {
    const mPluginDriver = {
        initPlugins: jest.fn(),
        getPluginContext: jest.fn(() => ({
            setContextPtr: jest.fn()
        })),
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
        LogDataFactory: {
            newInstance: jest.fn(() => ({
                code: '001', description: '', cause: '', position: '', solutions: [], moreInfo: {}
            }))
        }
    };
});
jest.mock('../../../src/pre_define', () => ({
    DECL_ETS_SUFFIX: '.d.ets',
    KOALA_WRAPPER_PATH_FROM_SDK: 'koala'
}));

const fakeArkts = {
    Config: { create: jest.fn(() => ({ peer: 'peer' })) },
    Context: { createCacheContextFromFile: jest.fn(() => ({ peer: 'contextPeer' })) },
    proceedToState: jest.fn(),
    Es2pandaContextState: {
        ES2PANDA_STATE_PARSED: 1,
        ES2PANDA_STATE_CHECKED: 2,
        ES2PANDA_STATE_BIN_GENERATED: 3,
        ES2PANDA_STATE_LOWERED: 4
    },
    generateStaticDeclarationsFromContext: jest.fn(),
    destroyConfig: jest.fn()
};
const fakeArktsGlobal = {
    es2panda: {
        _DestroyContext: jest.fn()
    },
    config: '',
    compilerContext: { peer: 'contextPeer' }
};

jest.mock('/sdk/koala', () => ({
    arkts: fakeArkts,
    arktsGlobal: fakeArktsGlobal
}), { virtual: true });

beforeEach(() => {
    jest.resetModules();
    jest.clearAllMocks();
    require('path').resolve.mockImplementation((...args: string[]) => {
        while (args.includes('koala')) return '/sdk/koala';
        return args.join('/');
    });
    require('path').join.mockImplementation((...args: string[]) => args.join('/'));
    require('path').relative.mockImplementation((from: string, to: string) => to.replace(from, '').replace(/^\//, ''));
    require('path').basename.mockImplementation((p: string) => p.split('/').pop());
});

// create a test to avoid throw error
describe('mockSDK', () => {
    it('should load correctly', () => {

    });
});

/* compile_thread_worker is'not used in the project now, so we comment out the test cases.
describe('compile_thread_worker', () => {
    let parentPort: EventEmitter & { postMessage?: jest.Mock };
    const workerData = { workerId: 1 };
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
    const jobInfo = {
        id: 123,
        isCompileAbc: true,
        buildConfig,
        compileFileInfo: fileInfo,
        globalContextPtr: 0
    };

    beforeEach(() => {
        parentPort = new EventEmitter() as any;
        parentPort.postMessage = jest.fn();
        jest.doMock('worker_threads', () => ({
            parentPort,
            workerData
        }));
    });

    afterEach(() => {
        jest.resetModules();
        jest.clearAllMocks();
        jest.dontMock('worker_threads');
    });

    test('compile abc && post TASK_FINISH', () => {
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo });

        expect(require('../../../src/utils').ensurePathExists).toHaveBeenCalledWith(fileInfo.abcFilePath);
        expect(fakeArkts.Config.create).toHaveBeenCalled();
        expect(fakeArkts.Context.createCacheContextFromFile).toHaveBeenCalled();
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(1, 'contextPeer');
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(2, 'contextPeer');
        expect(fakeArkts.proceedToState).toHaveBeenCalledWith(3, 'contextPeer');
        expect(fakeArkts.generateStaticDeclarationsFromContext).toHaveBeenCalled();
        expect(fakeArkts.destroyConfig).toHaveBeenCalled();
        expect(fakeArktsGlobal.es2panda._DestroyContext).toHaveBeenCalled();
        expect(parentPort.postMessage).toHaveBeenCalledWith(expect.objectContaining({
            type: 'TASK_FINISH',
            jobId: jobInfo.id,
            workerId: workerData.workerId
        }));
    });

    test('generate decl file', () => {
        let config = { ...buildConfig, hasMainModule: false };
        let job = { ...jobInfo, buildConfig: config };
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo: job });
        expect(fakeArkts.generateStaticDeclarationsFromContext).not.toHaveBeenCalled();
        config = { ...buildConfig, byteCodeHar: false, moduleType: 9999 };
        job = { ...jobInfo, buildConfig: config };
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo: job });
        expect(fakeArkts.generateStaticDeclarationsFromContext).not.toHaveBeenCalled();
    });

    test('call plugin hooks', () => {
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo });
        const pluginDriver = require('../../../src/plugins/plugins_driver').PluginDriver;
        expect(pluginDriver.initPlugins).toHaveBeenCalled();
        expect(pluginDriver.runPluginHook).toHaveBeenCalledWith(1);
        expect(pluginDriver.runPluginHook).toHaveBeenCalledWith(2);
        expect(pluginDriver.runPluginHook).toHaveBeenCalledWith(3);
    });

    test('handle error && printError', () => {
        fakeArkts.Config.create.mockImplementation(() => { throw new Error('fail'); });
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo });
        expect(require('../../../src/logger').Logger.printError).toHaveBeenCalled();
        expect(parentPort.postMessage).toHaveBeenCalledWith(expect.objectContaining({
            type: 'TASK_FINISH',
            jobId: jobInfo.id,
            workerId: workerData.workerId
        }));
    });

    test('handle compileExternalProgram', () => {
        const job = { ...jobInfo, isCompileAbc: false };
        require('../../../src/build/compile_thread_worker');
        parentPort.emit('message', { type: 'ASSIGN_TASK', jobInfo: job });
        const { arkts } = require('/sdk/koala');
        expect(arkts.Config.create).toHaveBeenCalled();
        expect(arkts.generateStaticDeclarationsFromContext).not.toHaveBeenCalled();
        expect(parentPort.postMessage).toHaveBeenCalledWith(expect.objectContaining({
            type: 'TASK_FINISH',
            jobId: jobInfo.id,
            workerId: workerData.workerId
        }));
    });

    test('exit on EXIT message', () => {
        const spy = jest.spyOn(process, 'exit').mockImplementation(() => { throw new Error('exit'); });
        require('../../../src/build/compile_thread_worker');
        expect(() => {
            parentPort.emit('message', { type: 'EXIT' });
        }).toThrow('exit');
        spy.mockRestore();
    });
   
});
 */