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
    One inherited explicit interface overload does not require redeclaration in a
    subinterface. The inherited overload remains usable through the subinterface.
---*/

interface I1 {
    fooInt(x: int): string
    fooString(x: string): string
    overload foo { fooInt, fooString }
}

interface I2 extends I1 {
}

class A implements I2 {
    fooInt(x: int): string {
        return "int"
    }

    fooString(x: string): string {
        return "string"
    }
}

function main(): void {
    let a: I2 = new A()
    arktest.assertEQ("int", a.foo(1))
    arktest.assertEQ("string", a.foo("ets"))
}
