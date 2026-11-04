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
    A subinterface that inherits several explicit overload declarations of the
    same name may remain valid when it declares a new explicit overload that
    includes all inherited target methods, even when the inherited member sets
    are disjoint and the new order differs.
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

interface I3 extends I1, I2 {
    overload foo { f4, f1, f3, f2 }
}

class A implements I3 {
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
}

function main(): void {
    let a: I3 = new A()
    arktest.assertEQ("f1", a.foo(1))
    arktest.assertEQ("f2", a.foo(true))
    arktest.assertEQ("f3", a.foo(1.5))
    arktest.assertEQ("f4", a.foo("ets"))
}
