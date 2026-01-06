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

import { WorkerMessageType, ProcessDeclgenV1Task } from '../../../src/types';
import { ErrorCode } from '../../../src/util/error';
import { LogData } from '../../../src/logger';

jest.mock('../../../src/util/ets2panda', () => {
    const mockEts2pandaInstance = {
        initalize: jest.fn(),
        declgenV1: jest.fn(),
        finalize: jest.fn(),
    };
    return {
        Ets2panda: {
            getInstance: jest.fn(() => mockEts2pandaInstance),
        },
    };
});

jest.mock('../../../src/util/utils', () => ({
    changeFileExtension: jest.fn(
        (file: string, ext: string) => `${file}.${ext}`
    ),
}));

jest.mock('path', () => ({
    resolve: jest.fn((...args: string[]) => args.join('/')),
    relative: jest.fn((from: string, to: string) => to.replace(from, '')),
    join: jest.fn((...args: string[]) => args.join('/')),
}));

type MockEts2panda = {
    initalize: jest.Mock;
    declgenV1: jest.Mock;
    finalize: jest.Mock;
};

describe('declgen_process_worker', () => {
    const mockTaskId = 'declgen-task-001';
    const mockBuildConfig = {
        cachePath: '/cache',
        skipDeclCheck: false,
        genDeclAnnotations: true,
    } as any;

    const mockFileInfo = {
        moduleName: 'test-module',
        moduleRoot: '/src',
        input: '/src/test.ets',
        output: '/cache/test.abc',
        arktsConfig: {},
    } as any;

    const baseTask: ProcessDeclgenV1Task = {
        fileInfo: mockFileInfo,
        fileList: ['/src/test.ets'],
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
        require('../../../src/build/declgen_process_worker');
    };

    test('should handle ASSIGN_TASK message and process single file', () => {
        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: baseTask },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.initalize).toHaveBeenCalled();
        expect(ets2panda.declgenV1).toHaveBeenCalledWith(
            baseTask,
            mockBuildConfig.skipDeclCheck ?? true,
            mockBuildConfig.genDeclAnnotations ?? true
        );
        expect(ets2panda.finalize).toHaveBeenCalled();
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.DECL_GENERATED,
            data: { taskId: mockTaskId },
        });
    });

    test('should process multiple files when fileList has more than one file', () => {
        const multiFileTask = {
            ...baseTask,
            fileList: ['/src/file1.ets', '/src/file2.ets'],
        };

        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: multiFileTask },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.declgenV1).toHaveBeenCalledTimes(2);
        expect(require('path').resolve).toHaveBeenCalledTimes(2);
        expect(ets2panda.finalize).toHaveBeenCalled();
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.DECL_GENERATED,
            data: { taskId: mockTaskId },
        });
    });

    test('should ignore messages with unknown type', () => {
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
                type: WorkerMessageType.DECL_GENERATED,
            })
        );
    });

    test('should use default values when skipDeclCheck and genDeclAnnotations are undefined', () => {
        const taskWithUndefinedOptions = {
            ...baseTask,
            buildConfig: {
                ...mockBuildConfig,
                skipDeclCheck: undefined,
                genDeclAnnotations: undefined,
            },
        };

        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: taskWithUndefinedOptions },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        const ets2panda = getMockEts2panda();
        expect(ets2panda.declgenV1).toHaveBeenCalledWith(
            taskWithUndefinedOptions,
            true, // Default for skipDeclCheck
            true // Default for genDeclAnnotations
        );
    });

    test('should handle DriverError and send ERROR_OCCURED message', () => {
        const mockErrorLog: LogData = {
            code: ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
            description: '',
            cause: '',
            position: '',
            solutions: [],
        };
        const realErrorModule = jest.requireActual('../../../src/util/error');
        const { DriverError } = realErrorModule;
        const mockDriverError = new DriverError(mockErrorLog);

        const ets2panda = getMockEts2panda();
        ets2panda.declgenV1.mockImplementation(() => {
            throw mockDriverError;
        });

        const message = {
            type: WorkerMessageType.ASSIGN_TASK,
            data: { taskId: mockTaskId, payload: baseTask },
        };

        loadWorkerModule();
        (process as any).emit('message', message);

        expect(ets2panda.finalize).toHaveBeenCalled();
        expect((process as any).send).toHaveBeenCalledWith({
            type: WorkerMessageType.ERROR_OCCURED,
            data: { taskId: mockTaskId, error: mockErrorLog },
        });
    });
});
