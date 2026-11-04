/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function foo(form?: String) {
    const f = (form == undefined) ? "abc" : form
    switch (f) {
        case "abc":
            return 41
        case "xyz":
            return 42
        default:
            return 0
    }
}

class A{}

function main(): void {
    arktest.assertEQ(foo(), 41,  "Error! foo() must be equal 41")
    arktest.assertEQ(foo("xyz"), 42,  "Error! foo(\"xyz\") must be equal 42")
    arktest.assertEQ(foo("NFD"), 0,  "Error! foo(\"NFD\") must be equal 0")
    let x = foo() == 41 ? new A() : undefined;
    arktest.assertTrue(x instanceof A)
    let y = foo("wxyz") == 42 ? "TRUE" : new A();
    arktest.assertTrue(y instanceof A)
    y = "JK";
    arktest.assertEQ(y, "JK")
}
