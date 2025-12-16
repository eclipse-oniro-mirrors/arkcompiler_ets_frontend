/*
* Copyright (c) 2022-2026 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import {launch} from "std/concurrency"

let count = 2
let n = 9
let a: int[] = new int[count];
let v: int[] = new int[count];
type P = Job<Int> | undefined
function assert_eq(value1: int, value2: int): void {
    if (value1 == value2) {
        return;
    }
    console.println("Values of type int are not equal: " + value1 + " != " + value2);
    throw new Error();
}
function ufib(n: int) : Int {
    if (n >= 0 && n < count) {
        return v[n];
    }
    let p: P[] = new P[count]
    for (let i = 0; i < count; ++i) {
        p[i] = launch<Int>() {
            return ufib(n-1-i)
        };
    }
    let result = 0
    for (let i = 0; i < count; ++i) {
        result = result + p[i]!.Await() * a[i];
    }
    return result;
}
function ufib_seq(n: int) : int {
    if (n >= 0 && n < count) {
        return v[n];
    }
    let result = 0
    for (let i = 0; i < count; ++i) {
        result = result + ufib_seq(n-1-i) * a[i];
    }
    return result;
}
export function main(): int {
    a[0] = 2;
    v[0] = 6;
    a[1] = 2;
    v[1] = 7;
    let seq_result = ufib_seq(n);
    let p = launch<Int>() {
        return ufib(n)
    };
    let co_result = p.Await();
    assert_eq(co_result as int, seq_result);
    return 0;
}
