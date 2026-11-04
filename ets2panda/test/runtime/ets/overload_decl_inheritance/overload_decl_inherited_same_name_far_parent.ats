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
    When a child adds a same-name method that does not match the current call,
    the inherited same-name method from a farther parent must remain available
    through the inherited overload alias.
---*/

interface I1 {
    foo1(a: long): string { return "long" }
    foo2(b: int): string { return "int" }
    overload foo { foo1, foo2 }
}

interface I2 extends I1 {
    foo1(a: string): string { return "string" }
}

let i = {} as I2

function main(): void {
    arktest.assertEQ(i.foo(1 as long), "long")
}
