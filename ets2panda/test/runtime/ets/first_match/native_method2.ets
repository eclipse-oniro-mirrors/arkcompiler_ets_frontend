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
    native foo1(x: number): void;
    foo2(x: string) {
        return "invoke1"
    }
    native foo3(): void;
    foo4(x: boolean) {
        return "invoke2"
    }
    overload foo{ foo1, foo2, foo3, foo4 }
}


function main() {
    let a = new A();
    arktest.assertEQ(a.foo("abc"), "invoke1");
    arktest.assertEQ(a.foo(true), "invoke2");
}
