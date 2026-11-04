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

function foo(x: A | null): boolean {
    if (x != null) {
        return x instanceof A
    }
    return false
}

function bar(): boolean {
    let x: A | null = null
    return x instanceof A
}

function baz(x: string | undefined): boolean {
    return x instanceof Object
}

function quz(x: string | int): boolean {
    return x instanceof Object
}

/* @@? 20:16 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 27:12 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as false */
/* @@? 35:12 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
