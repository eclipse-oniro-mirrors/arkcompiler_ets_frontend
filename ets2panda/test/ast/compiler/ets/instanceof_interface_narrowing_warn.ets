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

// Test that instanceof between an interface-typed variable and an unrelated
// class does NOT produce "always false" warning (W1001506), because a class
// implementing the interface may also extend the tested class.

interface I {
    foo(): void
}

class A {
    bar(): void {}
}

class C extends A implements I {
    foo(): void {}
}

class D {}

// interface variable instanceof unrelated class — no warning
function ifaceToClass(i: I): boolean {
    return i instanceof A
}

// interface variable instanceof unrelated interface — no warning
function ifaceToIface(i: I): boolean {
    return i instanceof D
}

// class variable instanceof unrelated interface — no warning
function classToIface(a: A): boolean {
    return a instanceof I
}

// class variable instanceof unrelated class — SHOULD warn "always false"
function classToClass(a: A): boolean {
    return a instanceof D
}

// Smart narrowing: after if (i instanceof A), i is narrowed to A,
// so i instanceof A is "always true"
function smartNarrowTrue(i: I): boolean {
    if (i instanceof A) {
        return i instanceof A
    }
    return false
}

// Smart narrowing: after if (i instanceof A), i is narrowed to A,
// then i instanceof D (unrelated class to A) is "always false"
function smartNarrowFalse(i: I): boolean {
    if (i instanceof A) {
        return i instanceof D
    }
    return false
}

// Smart narrowing: after if (i instanceof A), i is narrowed to A,
// then i instanceof I is NOT "always true"
function smartNarrowBackToIface(i: I): boolean {
    if (i instanceof A) {
        return i instanceof I
    }
    return false
}

/* @@? 51:12 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
/* @@? 58:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 67:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
