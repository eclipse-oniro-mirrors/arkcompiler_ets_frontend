/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { Executor } from './executor';
import type { DeclgenTask } from './task';
import { WorkerMessageType } from './taskManager';
import type { LogDataInit } from '@interop-toolkits/common';
import { ErrorCode } from '../../errors';

function execute(id: string, task: DeclgenTask): void {
  const executor = new Executor(task.buildConfig);
  executor.execute(task.inputFiles, task.arktsconfigPath);
  process.send?.({
    type: WorkerMessageType.DECL_GENERATED,
    data: {
      taskId: id,
    },
  });
}

interface AssignTaskMessage {
  type: WorkerMessageType.ASSIGN_TASK;
  data: {
    taskId: string;
    payload: DeclgenTask;
  };
}

process.on('message', (message: AssignTaskMessage) => {
  const { type, data } = message;
  if (type !== WorkerMessageType.ASSIGN_TASK) {
    return;
  }
  try {
    execute(data.taskId, data.payload);
  } catch (error) {
    const errorMessage: LogDataInit = {
      code: ErrorCode.STATIC_WORKER_PROCESS_FAILED,
      description: error instanceof Error ? error.message : String(error),
    };
    process.send?.({
      type: WorkerMessageType.ERROR_OCCURED,
      data: {
        taskId: data.taskId,
        error: errorMessage,
      },
    });
  } finally {
    process.send?.({
      type: WorkerMessageType.TASK_FINISHED,
      data: {
        taskId: data.taskId,
      },
    });
  }
});
