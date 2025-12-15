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

import { ErrorCode } from '../../../src/util/error';
import {
    WorkerMessageType,
    ProcessCompileTask,
    BUILD_MODE,
} from '../../../src/types';
import { LogData } from '../../../src/logger';

jest.mock('../../../src/util/ets2panda', () => {
    const mockEts2pandaInstance = {
        initalize: jest.fn(),
        compile: jest.fn(),
        compileSimultaneous: jest.fn(),
        finalize: jest.fn(),
    };
    return {
        Ets2panda: {
            getInstance: jest.fn(() => mockEts2pandaInstance),
            destroyInstance: jest.fn(),
        },
    };
});

type MockEts2panda = {
    initalize: jest.Mock;
    compile: jest.Mock;
    compileSimultaneous: jest.Mock;
    finalize: jest.Mock;
};

describe('compile_process_worker', () => {
    const mockTaskId = 'test-task-123';
    const mockBuildConfig = {
        buildMode: BUILD_MODE.DEBUG,
        dumpPerf: false,
    } as any;
    const mockFileInfo = { input: 'test.ets', output: 'test.abc' } as any;
    const mockDeclgenConfig = { output: 'test.d.ts' } as any;

    const baseTask: ProcessCompileTask = {
        fileInfo: mockFileInfo,
        fileList: ['test.ets'],
        declgenConfig: mockDeclgenConfig,
        type: 'DECL_ABC' as any,
        buildConfig: mockBuildConfig,
    } as any;

    const getMockEts2panda = (): MockEts2panda => {
        const { Ets2panda } = require('../../../src/util/ets2panda');
        return Ets2panda.getInstance() as MockEts2panda;
    };

    beforeEach(() => {
        jest.resetModules();
        (process as any).send = jest.fn();
        jest.spyOn(process, 'exit').mockImplementation(() => {
            throw new Error('exit');
        });
        jest.clearAllMocks();
    });

    afterEach(() => {
        jest.restoreAllMocks();
    });

    const loadWorkerModule = () => {
        require('../../../src/build/compile_process_worker');
    };

    test('compile_process_worker compile test', () => {
        const task = { ...baseTask, fileList: ['single.ets'] };
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: task },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.initalize).toHaveBeenCalled();
        expect(ets2panda.compile).toHaveBeenCalledWith(
            mockTaskId,
            task,
            true,
            expect.any(Function),
            expect.any(Function)
        );
        expect(ets2panda.compileSimultaneous).not.toHaveBeenCalled();
        expect(ets2panda.finalize).toHaveBeenCalled();
        expect(
            require('../../../src/util/ets2panda').Ets2panda.destroyInstance
        ).toHaveBeenCalled();
    });

    test('compile_process_worker compileSimultaneous test', () => {
        const task = { ...baseTask, fileList: ['file1.ets', 'file2.ets'] };
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: task },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.compileSimultaneous).toHaveBeenCalledWith(
            mockTaskId,
            task,
            false,
            true,
            expect.any(Function),
            expect.any(Function)
        );
        expect(ets2panda.compile).not.toHaveBeenCalled();
        expect(ets2panda.finalize).toHaveBeenCalled();
    });

    test('compile_process_worker decl callback is trigger test', () => {
        const task = { ...baseTask };
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: task },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        const declCallback = ets2panda.compile.mock.calls[0][3];
        declCallback();
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.DECL_GENERATED,
            data: { taskId: mockTaskId },
        });
    });

    test('compile_process_worker abc callback is trigger test', () => {
        const task = { ...baseTask };
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: task },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        const abcCallback = ets2panda.compile.mock.calls[0][4];
        abcCallback();
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.ABC_COMPILED,
            data: { taskId: mockTaskId },
        });
    });

    test('compile_process_worker unknown type test', () => {
        const message = {
            type: 'UNKNOWN_TYPE' as WorkerMessageType,
            data: { taskId: mockTaskId, payload: baseTask },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.initalize).not.toHaveBeenCalled();
        expect((process as any).send).not.toHaveBeenCalledWith(
            expect.objectContaining({
                type: expect.any(String),
            })
        );
    });

    const mockProcessSend = jest.fn();
    Object.defineProperty(process, 'send', {
        value: mockProcessSend,
        writable: true,
    });

    test('compile_process_worker catch branch test', () => {
        const mockErrorLog: LogData = {
            code: ErrorCode.BUILDSYSTEM_ABC_FILE_MISSING_IN_BCHAR,
            description: '',
            cause: '',
            position: '',
            solutions: [],
        };
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: baseTask },
        };
        loadWorkerModule();
        const ets2panda = getMockEts2panda();
        const realErrorModule = jest.requireActual('../../../src/util/error');
        const { DriverError } = realErrorModule;
        ets2panda.compile.mockImplementation(() => {
            throw new DriverError(mockErrorLog);
        });
        (process as any).emit('message', message);
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.ERROR_OCCURED,
            data: { taskId: mockTaskId, error: mockErrorLog },
        });
        expect(ets2panda.compile).toHaveBeenCalled();
    });
});
