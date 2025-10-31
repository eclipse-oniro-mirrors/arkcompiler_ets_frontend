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

class A {
    static foo(a: number): string {
        return "invoke1"
    };
    static foo(a: string): string {
        return "invoke2"
    };

    foo(a: number): string {
        return "invoke3"
    };
    foo(a: string): string {
        return "invoke4"
    };
}

function main() {
    let a = new A();
    arktest.assertEQ(a.foo(1), "invoke3")
    arktest.assertEQ(a.foo("aaa"), "invoke4")
    arktest.assertEQ(A.foo(1), "invoke1")
    arktest.assertEQ(A.foo("aaa"), "invoke2")
}
