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

class A{
    foo(a?: number, b?: string, f?: () => string) {
        arktest.assertEQ(a, a == undefined ? undefined : 1)
        arktest.assertEQ(b, b == undefined ? undefined : "1")
        if(f != undefined) {
            arktest.assertEQ(f(), "Ok")
        }
    }
}

function main() {
    let a = new A()
    a.foo() {
        return "Ok"
    }
    a.foo(1) {
        return "Ok"
    }
    a.foo(1, "1") {
        return "Ok"
    }
    a.foo(1, "1")
}