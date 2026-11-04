/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

type Callback = (() => void) | ((n: number) => void) | ((n: number, n2: number) => void)
function fn(callback: Callback): void {
    callback(1, 2)
}

// test for lambda as param.
let emptyArg: number = 0;
let oneArg: number = 0;
let twoArgs: number = 0;
fn(() => { emptyArg = emptyArg - 1; })
fn((n:number) => { oneArg = oneArg + n; })
fn((n:number, n2: number) => { twoArgs = twoArgs + n + n2; })
arktest.assertEQ(emptyArg, -1)
arktest.assertEQ(oneArg, 1)
arktest.assertEQ(twoArgs, 3)

// test for named function as param.
emptyArg = 0;
oneArg = 0;
twoArgs = 0;
function foo() { emptyArg = emptyArg - 1; }
function foo1(n: number) { oneArg = oneArg + n; }
function foo2(n:number, n2: number) { twoArgs = twoArgs + n + n2; }
fn(foo);
fn(foo1);
fn(foo2);
arktest.assertEQ(emptyArg, -1)
arktest.assertEQ(oneArg, 1)
arktest.assertEQ(twoArgs, 3)

// test for class method and static class method as param
class A {
    goo() { emptyArg = emptyArg - 1; }
    goo1(n: number) { oneArg = oneArg + n; }
    goo2(n:number, n2: number) { twoArgs = twoArgs + n + n2; }

    static zoo() { emptyArg = emptyArg - 1; }
    static zoo1(n: number) { oneArg = oneArg + n; }
    static zoo2(n:number, n2: number) { twoArgs = twoArgs + n + n2; }
}

let a = new A();
emptyArg = 0;
oneArg = 0;
twoArgs = 0;
fn(a.goo);
fn(a.goo1);
fn(a.goo2);
arktest.assertEQ(emptyArg, -1)
arktest.assertEQ(oneArg, 1)
arktest.assertEQ(twoArgs, 3)

emptyArg = 0;
oneArg = 0;
twoArgs = 0;
fn(A.zoo);
fn(A.zoo1);
fn(A.zoo2);
arktest.assertEQ(emptyArg, -1)
arktest.assertEQ(oneArg, 1)
arktest.assertEQ(twoArgs, 3)
