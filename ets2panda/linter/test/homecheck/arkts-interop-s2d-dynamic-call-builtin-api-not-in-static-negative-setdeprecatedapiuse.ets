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

import { set1, foo2 } from './fixtures/interop/s2d/builtin-api/exportEts2';
import { set11 } from './fixtures/interop/s2d/builtin-api/exportEts1';

class Ctx {
    log(value1: string, value2: string): void {
        console.log(value1, value2);
    }
}

function foo(value: string, key: string, set: Set<string>) {}

const setGlobal1 = set1;
const setGlobal2 = setGlobal1;

function testSetForEachAPI(param1: Set<string>, param2: Set<string>): void {
    set1.forEach(foo, new Ctx());                       // 告警1
    const setFromImport = set1;
    const set2 = setFromImport;
    set2.forEach(foo, new Ctx());                       // 告警2
    setGlobal2.forEach(foo, new Ctx());                 // 告警3
    const setFromGlobal = setGlobal1;
    setFromGlobal.forEach(foo, new Ctx());              // 告警4
    const setFromReturnValue = foo2();
    setFromReturnValue.forEach(foo, new Ctx());         // 告警5
    param1.forEach(foo, new Ctx());                     // 告警6
    param2.forEach(foo, new Ctx());                     // 告警7
}

testSetForEachAPI(setGlobal2, set11);
testSetForEachAPI(set11, set1);

let setGlobal11 = set1;
setGlobal11 = set11;

function positiveSetEveryAPI(): void {
    set11.forEach(foo, new Ctx());          // 不告警
    const set1 = new Set<[1, 2, 3]>;
    set1.forEach(foo, new Ctx());           // 不告警
    setGlobal11.forEach(foo, new Ctx());    // 不告警
    setGlobal1.forEach(foo);                // 不告警
}

function test1(): Set<number> {
    const res = setGlobal2
    return res;
}

function test2(): Set<number> {
    return test1();
}

function test3(): Set<number> {
    return test2();
}

function testWithOneCallSite() {
    test1().forEach(foo, new Ctx());        // 告警8
}

function testWithTwoCallSite() {
    test2().forEach(foo, new Ctx());        // 告警9
}

function testWithThreeCallSite() {
    test3().forEach(foo, new Ctx());        // 告警10
}

class TestCrossMethod {
    test1(param1: Set<number>) {
        param1.forEach(foo, new Ctx());     // 告警11
    }

    test2(param2: Set<number>) {
        this.test1(param2);
    }

    test3(param3: Set<number>) {
        this.test2(param3);
    }
}

let testCrossMethod = new TestCrossMethod();
testCrossMethod.test3(set1);
