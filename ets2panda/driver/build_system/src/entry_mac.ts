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

import { LoggerGetter } from './logger';
import { BuildConfig } from './types';
import { runBuild } from './entry';

enum MessageType {
    BUILD = 'BUILD',
}

interface MainToSubMsg {
    task: MessageType;
    config: BuildConfig;
    loggerGetter?: LoggerGetter;
}

interface SubToMainMsg {
    success: boolean;
    errMsg?: string;
}

/**
 * main process
 */
export async function buildForMac(
    projectConfig: BuildConfig,
    loggerGetter?: LoggerGetter
): Promise<void> {
    return new Promise((resolve, reject) => {
        // 1. create child process with current file , execute if (process.send) below
        const child = fork(path.resolve(__filename), [], { stdio: 'inherit' });

        // 2. send build msg to child process
        child.send({
            task: MessageType.BUILD,
            config: projectConfig,
            loggerGetter,
        } as MainToSubMsg);

        // 3. response child msg
        child.on('message', (rawMsg: unknown) => {
            const msg = rawMsg as SubToMainMsg;
            if (msg.success) {
                resolve();
            } else {
                reject(new Error(msg.errMsg));
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
        const msg = rawMsg as MainToSubMsg;
        if (msg.task === MessageType.BUILD) {
            try {
                // build in child process and response to main process
                await runBuild(msg.config, msg.loggerGetter);
                process.send!({ success: true } as SubToMainMsg);
                process.exit(0);
            } catch (e) {
                const err = e as Error;
                process.send!({
                    success: false,
                    errMsg: err.message,
                } as SubToMainMsg);
                process.exit(1);
            }
        }
    });
}
