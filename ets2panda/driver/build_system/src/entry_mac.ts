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
import { fork } from 'child_process';

import { runBuild } from './entry';
import { BuildConfig, WorkerMessageType, WorkerMessage, WorkerLogMessage, WorkerToMainMessage, MainToWorkerMessage } from './types';
import { getInterProcessLogger, Logger } from './logger';
import { handleLogMessage } from './util/logger_util';

/**
 * main process
 */
export async function buildForMac(
    projectConfig: BuildConfig,
): Promise<void> {
    return new Promise((resolve, reject) => {
        // 1. create child process with current file , execute if (process.send) below
        const child = fork(path.resolve(__filename), [], { stdio: 'inherit' });

        // 2. send build msg to child process
        child.send({
            type: WorkerMessageType.BUILD,
            config: projectConfig,
        } as MainToWorkerMessage);

        // 3. response child msg
        child.on('message', (rawMsg: unknown) => {
            const msg = rawMsg as WorkerMessage;
            if (msg.type === WorkerMessageType.SUB_RESPONSE) {
                const rspMsg = msg as WorkerToMainMessage;
                if (rspMsg.success) {
                    resolve();
                } else {
                    reject(new Error(rspMsg.errMsg));
                }
            }

            if (msg.type === WorkerMessageType.LOG) {
                handleLogMessage(Logger.getInstance(), msg as WorkerLogMessage);
            }
        });

        // 4. response child close msg
        child.on('close', (code: number | null) => {
            if (code !== 0) {
                reject(new Error(`fork subprocess failed;code=${code}`));
            }
        });

        // 5. response create child process failed
        child.on('error', (err: Error) => reject(err));
    });
}

/**
 * child process
 */
if (process.send) {
    process.on('message', async (rawMsg: unknown) => {
        const msg = rawMsg as WorkerMessage;
        if (msg.type === WorkerMessageType.BUILD) {
            let mainMsg = msg as MainToWorkerMessage;
            try {
                // build in child process and response to main process
                // init sub process logger , print log to main process
                Logger.getInstance(getInterProcessLogger, mainMsg.config.enableDebugOutput);
                await runBuild(mainMsg.config);
                process.send!({ type: WorkerMessageType.SUB_RESPONSE, success: true } as WorkerToMainMessage);
                process.exit(0);
            } catch (e) {
                const err = e as Error;
                process.send!({
                    type: WorkerMessageType.SUB_RESPONSE,
                    success: false,
                    errMsg: err.message,
                } as WorkerToMainMessage);
                process.exit(1);
            }
        }
    });
}
