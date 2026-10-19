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

import { systemDateTime } from '@ohos.int';

let arr = [1, 2, 3];

async function asyncFunc1() {
    return 1; // arkts-numeric-semantic
}

async function asyncFunc2(): Promise<number> {
    return 2; // arkts-numeric-semantic
}

async function callAsyncFunc() {
    let a = await asyncFunc1() / await asyncFunc2(); // arkts-numeric-semantic
    let b = arr[await asyncFunc2()]; // arkts-numeric-semantic
    return b;
}

class AsyncMethod {
    async asyncMethod1() {
        let a1 = 1; // arkts-numeric-semantic
        let b1 = a1; // arkts-numeric-semantic
        return b1;
    }

    async asyncMethod2(): Promise<number> {
        let a2 = 1; // arkts-numeric-semantic
        a2 = await this.asyncMethod1();
        let b2 = a2; // arkts-numeric-semantic
        return b2;
    }

    async asyncMethod3(): Promise<number> {
        let a3 = 1; // arkts-numeric-semantic
        let b3 = a3 + 1; // arkts-numeric-semantic
        return b3;
    }

    async asyncMethod4(con: boolean) {
        if (con) {
            return 1 / 2; // arkts-numeric-semantic
        } else {
            let a4 = systemDateTime.getTime(); // arkts-numeric-semantic
            return a4;
        }
    }
}

class AsyncLambda {
    lambda1() {
        let func1 = async () => { return 1 }; // arkts-numeric-semantic
        let a5 = func1();
    }

    async lambda2() {
        let func2 = async (): Promise<number> => { return 2 }; // arkts-numeric-semantic
        let a6 = await func2();
    }
}

class AsyncCallback {
    private a = 0; // arkts-numeric-semantic
    private b = 0; // arkts-numeric-semantic

    async foo<T>(callback: () => T | Promise<T>): Promise<T> {
        return callback();
    }

    async func1(): Promise<number> {
        return this.foo(() => {
            return this.a;
        });
    }

    async func2(): Promise<number> {
        return this.foo(() => {
            let c = 1; // arkts-numeric-semantic
            return c;
        });
    }
}
