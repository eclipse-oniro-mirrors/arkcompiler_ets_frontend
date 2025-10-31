/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class Test {
    foo1(a: byte): string { return "invoke1" }
    foo2(a: int): string { return "invoke2" }
    foo3(a: float): string { return "invoke3" }
    foo4(a: number): string { return "invoke4" }
    foo5(a: char): string { return "invoke5" }
    foo6(a: string): string { return "invoke6" }
    foo7(a: Byte): string { return "invoke7" }
    foo8(a: Int): string { return "invoke8" }
    foo9(a: Number): string { return "invoke9" }
    foo10(a: Float): string { return "invoke10" }

    overload foo{ foo1, foo2, foo3, foo4, foo5, foo6, foo7, foo8, foo9, foo10 }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo(1), "invoke1")
    arktest.assertEQ(a.foo(1 as byte), "invoke1")
    arktest.assertEQ(a.foo(1 as int), "invoke2")
    arktest.assertEQ(a.foo(1 as number), "invoke4")
    arktest.assertEQ(a.foo(1.0), "invoke3")
    arktest.assertEQ(a.foo(1.0 as float), "invoke3")
    arktest.assertEQ(a.foo('a'), "invoke6")
    arktest.assertEQ(a.foo("abcd"), "invoke6")

    let param1: Int = 1;
    let param2: Number = 1;
    let param3: Float = 1;
    arktest.assertEQ(a.foo(param1), "invoke2")
    arktest.assertEQ(a.foo(param2), "invoke4")
    arktest.assertEQ(a.foo(param3), "invoke3")
}
