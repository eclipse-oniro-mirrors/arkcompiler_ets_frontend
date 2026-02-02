/**
 * Copyright (c) 2025 - 2026 Huawei Device Co., Ltd.
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

import {
    WorkerMessageType,
    ProcessCompileTask,
    JobContentType,
} from '../types';
import { LogDataFactory, LogData, Logger, getConsoleLogger } from '../logger';
import { ErrorCode, DriverError } from '../util/error';
import { Ets2panda } from '../util/ets2panda';


const logger = Logger.getInstance(getConsoleLogger)

function compile(id: string, task: ProcessCompileTask): void {

    const ets2panda = Ets2panda.getInstance(task.buildConfig);

    const declGeneratedCb = (): void => {
        process.send!({
            type: WorkerMessageType.DECL_GENERATED,
            data: {
                taskId: id,
            }
        });
    }

    const abcCompiledCb = (): void => {
        process.send!({
            type: WorkerMessageType.ABC_COMPILED,
            data: {
                taskId: id,
            }
        });
    }

    try {
        ets2panda.initalize();
        if (task.contentType === JobContentType.CLUSTER) {
            ets2panda.compileSimultaneous(
                id,
                task,
                true,
                declGeneratedCb,
                abcCompiledCb
            )
        } else {
            ets2panda.compile(
                id,
                task,
                declGeneratedCb,
                abcCompiledCb
            )
        }
    } catch (error) {
        if (error instanceof DriverError) {
            process.send!({
                type: WorkerMessageType.ERROR_OCCURED,
                data: {
                    taskId: id,
                    error: error.logData
                }
            });
        }
    } finally {
        ets2panda.finalize();
        process.send!({
            type: WorkerMessageType.TASK_FINISHED
        });
    }

    Ets2panda.destroyInstance();
}

process.on('message', (message: {
    type: WorkerMessageType,
    data: {
        taskId: string,
        payload: ProcessCompileTask
    }
}) => {
    const { type, data } = message;
    logger.printDebug(`Got message from parent. Type: ${type}. TaskId ${data.taskId}`)
    try {
        switch (type) {
            case WorkerMessageType.ASSIGN_TASK:
                compile(data.taskId, data.payload);
                break;
            default:
                break;
        }
    } catch (error) {
        logger.printDebug('Error occured');
        if (error instanceof Error) {
            let logData: LogData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                'Compile abc files failed.',
                error.message,
            );
            process.send!({
                type: WorkerMessageType.ERROR_OCCURED,
                data: {
                    taskId: data.taskId,
                    error: logData
                }
            });
        }
        logger.printDebug('Sent the error to the parent');
    }
});
