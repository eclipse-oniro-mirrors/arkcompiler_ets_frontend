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

class Test<T> {
    foo1(a: T[]): string {
        return "invoke1";
    }
    foo2(a: string[]): string {
        return "invoke2";
    }
    foo3(a: Array<T>): string {
        return "invoke3";
    }
    foo4(a: Array<string>): string {
        return "invoke4";
    }
    foo5(a: T[][]): string {
        return "invoke5";
    }
    foo6(a: string[][]): string {
        return "invoke6";
    }

    overload foo{ foo1, foo2, foo3, foo4, foo5, foo6 }
}

function main() {
    let a: Test<number> = new Test<number>();
    arktest.assertEQ(a.foo([123, 123, 123]), "invoke1");
    arktest.assertEQ(a.foo(new number[5]), "invoke1");
    arktest.assertEQ(a.foo(new Array<number>(2)), "invoke1");
    arktest.assertEQ(a.foo(new number[5][10]), "invoke5");

    arktest.assertEQ(a.foo(["abc", "abc", "abc"]), "invoke2");
    arktest.assertEQ(a.foo(new string[5]), "invoke2");
    arktest.assertEQ(a.foo(new Array<string>(2)), "invoke2");
    arktest.assertEQ(a.foo(new string[5][10]), "invoke6");
}
