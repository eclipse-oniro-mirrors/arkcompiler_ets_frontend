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
    foo1(a: [string, string, string]): string {
        return "invoke1";
    }
    foo2(a: [number, string, boolean]): string {
        return "invoke2";
    }
    foo3(a: number[]): string {
        return "invoke3";
    }

    overload foo{ foo1, foo2, foo3 }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo(["abc", "abc", "abc"]), "invoke1");
    arktest.assertEQ(a.foo([123, "abc", true]), "invoke2");
    arktest.assertEQ(a.foo([123, 123, 123]), "invoke3");

    let param1: [string, string, string] = ["abc", "abc", "abc"];
    let param2: [number, string, boolean] = [123, "abc", true];
    let param3: number[] = [123, 123, 123]
    arktest.assertEQ(a.foo(param1), "invoke1");
    arktest.assertEQ(a.foo(param2), "invoke2");
    arktest.assertEQ(a.foo(param3), "invoke3");
}
