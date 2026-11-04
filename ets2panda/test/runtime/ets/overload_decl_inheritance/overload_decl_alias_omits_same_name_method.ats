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

/*---
desc: >-
    A same-name method introduced below the explicit overload declaration is
    prepended even when the inherited overload list itself does not mention that
    method name.
---*/

interface Base {
    fooInt(p: int): string { return "int" }
    overload foo { fooInt }
}

interface Derived extends Base {
    foo(p: byte): string { return "byte" }
}

class Impl implements Derived {
    override foo(p: byte): string { return "byte" }
    override fooInt(p: int): string { return "int" }
}

function main(): void {
    let d: Derived = new Impl()
    arktest.assertEQ(d.foo(1 as byte), "byte")
    arktest.assertEQ(d.foo(1 as int), "int")
}
