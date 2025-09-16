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

namespace NS {
    export function foo1(a: string): string {
        return "invoke1"
    };
    export function foo2(a: boolean): string {
        return "invoke2"
    };
}

namespace NS2 {
    export function foo3(a: number): string {
        return "invoke3"
    }
}

class A { }

function foo4(a: A): string {
    return "invoke4"
};

overload foo{ foo4, NS.foo1, NS.foo2, NS2.foo3 };

function main() {
    arktest.assertEQ(foo("123"), "invoke1")
    arktest.assertEQ(foo(true), "invoke2")
    arktest.assertEQ(foo(1), "invoke3")
    arktest.assertEQ(foo(new A()), "invoke4")
}
