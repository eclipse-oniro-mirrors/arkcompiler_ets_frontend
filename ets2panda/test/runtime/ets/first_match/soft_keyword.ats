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


function type1(a: number) { return "invoke1" }
function type2(a: string) { return "invoke2" }
overload type{ type1, type2 }

class A {
    type1(a: number) { return "invoke1" }
    type2(a: string) { return "invoke2" }
    overload type{ type1, type2 }
}

interface I {
    type1(a: number): string
    type2(a: string): string
    overload type{ type1, type2 }
}

function main(): void {
    arktest.assertEQ(type(123), "invoke1")
    arktest.assertEQ(type("abc"), "invoke2")

    let a = new A();
    arktest.assertEQ(a.type(123), "invoke1")
    arktest.assertEQ(a.type("abc"), "invoke2")
}
