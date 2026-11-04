/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

// #15380

// interface PromiseLike<T> {
//     then<U, V>(onFulfilled: (value: T) => U, onRejected: (error: Object | null) => V): PromiseLike<U | V>;
// }

// class TestP<T> implements PromiseLike<T> {
//     then<U, V>(onFulfilled: (value: T) => U, onRejected: (error: Object | null) => V): TestP<U | V> {
//         return new TestP<U | V>();
//     }
// }

// function testp() {
//     let cb1: (v: number | Array<number>) => Number = (v) => 1;
//     let cb2: (v: Object | null) => Error = (v) => new Error();
//     let p: PromiseLike<number | Array<number>> = new TestP<number | Array<number>>();
//     p.then<Number, Error>(cb1, cb2);
// }

function foo<T>(val: T | () => T)) {
    return true
}

class X<T> {
    method(...items: (T | X<T>)[]): X<T> {
        return new X<T>();
    }
}

export class TestPromiseFulfilledResult<T> {
    status: string;
    value: T;
}

export class TestPromiseRejectedResult {
    status: string;
    reason: Object;
}

type TestPromiseSettledResult<T> = TestPromiseFulfilledResult<T> | TestPromiseRejectedResult;

function functionOverValue<Value>(value: Value | () => Value)): boolean {
    return Type.of(value) instanceof FunctionType
}

function main() {
    foo(123);
    // foo()=>123); // union inference #15577

    let x = new X<number>();
    x.method(x, 123, x.method(321));

    let res: TestPromiseSettledResult<number | Error> = new TestPromiseFulfilledResult<number | Error>();

    arktest.assertTrue(!functionOverValue(123))
    arktest.assertTrue(functionOverValue(() => 123) as () => number))
}
