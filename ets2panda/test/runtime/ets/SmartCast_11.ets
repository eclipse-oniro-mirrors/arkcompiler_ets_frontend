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

class C {}

function foo(arg?: boolean): C | undefined {
    if (arg == undefined) {
        return undefined;
    }
    return arg == false ? undefined : new C();
}

function bar(arg?: boolean): C | undefined {
    if (arg == true) return new C();
    return arg == false ? undefined : new C();
}

function baz(arg?: boolean): int {
    if (arg == undefined) return 0;
    assertEQ(typeof arg, "boolean")
    return 1;
}

function main() {
    assertTrue(foo(true) instanceof C)
    assertEQ(foo(false), undefined)
    assertEQ(foo(), undefined)

    assertTrue(bar(true) instanceof C)
    assertEQ(bar(false), undefined)
    assertTrue(bar() instanceof C)

    assertEQ(baz(true), 1)
    assertEQ(baz(false), 1)
    assertEQ(baz(), 0)
}