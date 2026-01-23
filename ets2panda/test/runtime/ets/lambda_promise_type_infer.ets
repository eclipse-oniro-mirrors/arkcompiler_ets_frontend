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

async function test1<T>(cb: () => Promise<T>) {
    arktest.assertTrue(cb() instanceof Promise);
}

async function test2(cb: () => PromiseLike<string>) {
    arktest.assertTrue(cb() instanceof Promise);
}

async function test3(cb: () => PromiseLike<number>) {
    arktest.assertTrue(cb() instanceof Promise);
}

async function test4(cb: () => Promise<long>) {
    arktest.assertTrue(cb() instanceof Promise);
}

async function test5(cb: () => PromiseLike<long>) {
    arktest.assertTrue(cb() instanceof Promise);
}


function main() {
    test1(async () => { return 'str' });
    test1(async () => { return 1 });
    test1(() => { return new Promise<string>(() => { }); });
    test2(async () => { return 'str' });
    test3(async () => { return 1 });
    test4(async () => { return 1 });
    test5(async () => { return 1 });

    let func1 = async () => {
        const nestedAsync = async () => {
            return new Promise<string>((resolve) => {
                resolve("Hello from nested async function");
            });
        };

        const result = await Promise.resolve()
            .then(async () => {
                return await nestedAsync();
            });
        arktest.assertTrue(result == "Hello from nested async function");
    }
    func1();
}
