/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

import { TaskGroup, applyToUnknown, fooExecute } from "./dynamic_lib"
import {CallbackType, callback} from "./oh_modules/ohos_lib"

export declare function applyToUnknownETS(fn: (a: unknown) => void);

function printArg(arg: number): number {
    console.log(arg);
    return arg;
}

function main(): void {
    applyToUnknown((x: number) => { });

    applyToUnknownETS((x: number) => { });

    fooExecute(printArg, 1).then((value: number) => {
        console.log(value);
    });
    fooExecute(printArg, 1).then((value: Object[]) => {
        console.log(value);
    });
}

let taskGroup: TaskGroup = new TaskGroup();

fooExecute(taskGroup).then((res: number[]) => {
    console.log(res);
});

fooExecute(taskGroup).then((res: Array<number>) => {
    console.log(res);
});

fooLocal(callback);
fooLocal(callback);
