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

class A {}
class B extends A {}
class C {}
class D {}
class E {}

function foo(v: string): boolean {
    return v instanceof string | number
}

function bar(a: C): boolean {
    return a instanceof D | E
}

function baz(a: A): boolean {
    return a instanceof B | C
}

/* @@? 23:12 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 27:12 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
