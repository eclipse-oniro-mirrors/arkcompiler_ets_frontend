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


class A<T extends Object> {
    public a : T;
    constructor(a: T) {
        this.a = a;
    }
    bar() {
        fooB(this.a);
    }
}

function fooB<T>(a : T) {
    arktest.assertEQ(a, 10);
}

function testA<T extends int>(a: T) : void {
    arktest.assertEQ(a, 5);
}

function testB<S extends int>(a: S) : void {
    testA(a);
}

function testC<T extends S, S extends int>(a: T, b: S) : void {
    arktest.assertEQ(a, 8);
}

function main() {
    let a = new A<number>(10);
    a.bar();

    testB(5);
    testC(8, 1);
}
