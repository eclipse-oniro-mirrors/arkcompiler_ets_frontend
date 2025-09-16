#!/usr/bin/env node
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

const { spawn } = require('child_process');

const child = spawn(process.argv[2], process.argv.slice(3), {
    stdio: 'inherit',
    detached: true,
    windowsHide: true
});

const timeout = setTimeout(() => {
    console.error('process timeout');
    child.kill('SIGKILL');
    process.exit(124);
}, 900000);

child.on('exit', (code, signal) => {
    clearTimeout(timeout);

    if (signal === 'SIGSEGV' || signal === 'SIGABRT') {
        console.error(`process crashe: ${signal}`);
        process.exit(128 + signal);
    } else {
        process.exit(code ?? 0);
    }
});

child.on('error', (err) => {
    clearTimeout(timeout);
    console.error(`Promoter process failure: ${err.message}`);
    process.exit(127);
});

child.unref();