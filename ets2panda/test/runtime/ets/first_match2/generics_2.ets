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
    field: string = "A"
}

class Test<T> {
    foo(a: string): string {
        return "invoke1"
    }
    foo<T1 = T>(a: T1): string {
        return "invoke2"
    }
    foo<T1, T2>(a: T1, b: T2): string {
        return "invoke3"
    }
}

function main() {
    let a: Test<A> = new Test<A>();
    arktest.assertEQ(a.foo(new A()), "invoke2");
    arktest.assertEQ(a.foo<number>(10), "invoke2");
    arktest.assertEQ(a.foo<number, string>(10, "abc"), "invoke3");
    arktest.assertEQ(a.foo(10), "invoke2");
    arktest.assertEQ(a.foo(10, "abc"), "invoke3");
}
