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

function test7(cb: () => Promise<number> | Promise<String>) {
    arktest.assertTrue(cb() instanceof Promise);
}

function test8(cb: () => Promise<int> | Promise<String>) {
    arktest.assertTrue(cb() instanceof Promise);
}

function test9(cb: () => Promise<never> | Promise<String>) {
    arktest.assertTrue(cb() instanceof Promise);
}

function test10(cb: () => Promise<never>) {
    arktest.assertTrue(cb() instanceof Promise);
}

class A {}
class B extends A {}

interface I {}
class D implements I {}

function test11(cb: () => Promise<A>) {
    arktest.assertTrue(cb() instanceof Promise);
}

function test12(cb: () => Promise<I>) {
    arktest.assertTrue(cb() instanceof Promise);
}

async function test13(cb: (a: boolean) => Promise<int>) {
    arktest.assertEQ(await cb(true), 2);
    arktest.assertEQ(await cb(false), 1);
}

async function main() {
    test1(async () => { return 'str' });
    test1(async () => { return 1 });
    test1(() => { return new Promise<string>(() => { }); });
    test2(async () => { return 'str' });
    test3(async () => { return 1 });
    test4(async () => { return 1 });
    test5(async () => { return 1 });
    test7(async () => { return 1 });
    test7(async () => { return 's' });
    test8(async () => { return 1 });
    test8(async () => { return 's' });

    // #34629
    // let a = async () => { throw new Error }
    // test9(a);
    // test10(async () => { throw new Error });

    test11(async () => { return new A });
    test11(async () => { return new B });
    test12(async () => { return new D });
    let i : I = new D
    test12(async () => { return i });

    let al1 = async (a: boolean) => {if (a) {return 2} else {return Promise.resolve(1)}}
    await test13(al1)
    await test13(async (a: boolean) => {if (a) {return 2} else {return Promise.resolve(1)}})

    let al2 = async (a: boolean) => {if (!a) {return Promise.resolve(1)} else {return 2}}
    await test13(al2)
    await test13(async (a: boolean) => {if (!a) {return Promise.resolve(1)} else {return 2}})

    // currently, spec prohibits such function, as:
    // 16.3.1: An asynchronous function with the return type Promise<T> can explicitly return a Promise<T> instance
    // (in this case, the returned value is returned "as is") or a value of type T, which is then automatically
    // boxed in an instance of Promise<T>
    // let al3 = async (a: boolean) => {let res : int | Promise<int> = a ? 2 : Promise.resolve(1); return res; }
    // await test13(al3)
    // await test13(async (a: boolean) => {let res : int | Promise<int> = a ? 2 : Promise.resolve(1); return res; })
}
