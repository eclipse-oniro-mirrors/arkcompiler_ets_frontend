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

interface I {
    name: string
}

class C implements I {
    name: string = "C"
}

abstract class Base {
    name: string = ""
    abstract getName(): string
}

class Derived extends Base {
    override getName(): string { return "Derived" }
}

function main(): void {
    // Interface element type with explicit initializer
    let ifaceArr: FixedArray<I> = new FixedArray<I>(3, new C())
    arktest.assertEQ(ifaceArr.length, 3)
    arktest.assertTrue(ifaceArr[0] instanceof C)
    arktest.assertEQ(ifaceArr[0].name, "C")
    arktest.assertEQ(ifaceArr[2].name, "C")

    // Abstract class element type with explicit initializer
    let abstractArr: FixedArray<Base> = new FixedArray<Base>(2, new Derived())
    arktest.assertEQ(abstractArr.length, 2)
    arktest.assertTrue(abstractArr[0] instanceof Derived)
    arktest.assertEQ(abstractArr[0].getName(), "Derived")
    arktest.assertEQ(abstractArr[1].getName(), "Derived")
}