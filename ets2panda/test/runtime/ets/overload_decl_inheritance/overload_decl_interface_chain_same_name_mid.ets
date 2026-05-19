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
    A same-name method declared on an intermediate interface is collected before
    an inherited explicit overload from a more distant superinterface.
---*/

interface Root {
    f1(p: int): string { return "int" }
    f2(p: string): string { return "string" }
    overload foo { f1, f2 }
}

interface Mid extends Root {
    foo(p: byte): string { return "byte" }
}

class Impl implements Mid {
    f1(p: int): string { return "int" }
    f2(p: string): string { return "string" }
    override foo(p: byte): string { return "byte" }
}

function main(): void {
    let m: Mid = new Impl()
    arktest.assertEQ(m.foo(1 as byte), "byte")
    arktest.assertEQ(m.foo(1 as int), "int")
    arktest.assertEQ(m.foo("x"), "string")
}
