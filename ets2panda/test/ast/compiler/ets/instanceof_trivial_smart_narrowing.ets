/**
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

interface I {
    bar(): void
}

class C {
    run(): void {}
}

class A {}
class B extends A {
    m(): void {}
}
class D {}

function foo(i: I): boolean {
    if (i instanceof C) {
        return i instanceof C
    }
    return false
}

function bar(a: A): boolean {
    if (a instanceof B) {
        return a instanceof B
    }
    return false
}

function baz(a: A): boolean {
    if (a instanceof B) {
        return a instanceof D
    }
    return false
}

function qux(a: A): boolean {
    return a instanceof B
}

function quux(x: string | undefined): number {
    if (x == undefined) {
        return 0
    }
    let t1 = x instanceof string
    let t2 = x instanceof String
    return x.length + (t1 ? 1 : 0) + (t2 ? 1 : 0)
}

/* @@? 31:9 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
/* @@? 32:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 39:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 46:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
/* @@? 59:14 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 60:14 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
