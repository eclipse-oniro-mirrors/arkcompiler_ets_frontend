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

'use static'

import { calledByEts2 } from './IndexWithInt';

export let export2IndexInt = 2;
export let export2IndexNumber = 2.2;

export function export2FunctionInt(d: number): number {
    const a = 2;
    if (a > 0) {
        return d;
    } else {
        return -d;
    }
}

export function export2FunctionNumber(): number {
    let d = 1;
    if (d > 0) {
        return d;
    } else {
        return d + 0.3;
    }
}

function foo(): void {
    let a = 1;
    calledByEts2(a);
}