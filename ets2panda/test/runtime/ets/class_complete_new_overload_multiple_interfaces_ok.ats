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
    A class that implements interfaces with explicit overloads of the same name
    remains valid when it declares a new explicit overload containing all
    inherited target methods.
---*/

interface I1 {
    f1(x: int): string
    f2(x: boolean): string
    overload foo { f1, f2 }
}

interface I2 {
    f3(x: number): string
    f4(x: string): string
    overload foo { f3, f4 }
}

class A implements I1, I2 {
    f1(x: int): string {
        return "f1"
    }

    f2(x: boolean): string {
        return "f2"
    }

    f3(x: number): string {
        return "f3"
    }

    f4(x: string): string {
        return "f4"
    }

    overload foo { f2, f4, f1, f3 }
}

function main(): void {
    let a: A = new A()
    arktest.assertEQ("f1", a.foo(1))
    arktest.assertEQ("f2", a.foo(false))
    arktest.assertEQ("f3", a.foo(1.5))
    arktest.assertEQ("f4", a.foo("ets"))
}
