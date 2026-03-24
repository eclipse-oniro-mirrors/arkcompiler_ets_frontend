/*
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

import { parentPort } from 'worker_threads';
import {
    WorkerMessageType,
    ProcessCompileTask,
} from '../types';
import {
    LogData,
    LogDataFactory,
} from '../logger';
import { ErrorCode } from '../util/error';

parentPort!.on('message', (message: {
    type: WorkerMessageType,
    data: {
        taskId: string,
        payload: ProcessCompileTask
    }
}) => {
    const { data: data } = message;
    let logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Not implemented yet.',
    );
    process.send!({
        type: WorkerMessageType.ERROR_OCCURED,
        data: {
            taskId: data.taskId,
            error: logData
        }
    });
});
