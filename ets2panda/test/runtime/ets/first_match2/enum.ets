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

enum TestEnum1 {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
}

enum TestEnum2 {
    A = "A",
    B = "B",
    C = "C",
    D = "D",
}

class Test {
    foo(a: TestEnum1): string {
        return "invoke1"
    }
    foo(a: TestEnum2): string {
        return "invoke2"
    }
    foo(a: TestEnum1 | TestEnum2): string {
        return "invoke3"
    }
}

class Test2 {
    foo(a: string, b: TestEnum1): string {
        return "invoke1"
    }
    foo(a: number, b: TestEnum2): string {
        return "invoke2"
    }
    foo(a: string | number, b: TestEnum1 | TestEnum2): string {
        return "invoke3"
    }
}

class Test3 {
    foo(a: string): string {
        return "invoke1"
    }
    foo(a: number): string {
        return "invoke2"
    }
    foo(a: string | number): string {
        return "invoke3"
    }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo(TestEnum1.A), "invoke1");
    arktest.assertEQ(a.foo(TestEnum2.B), "invoke2");
    arktest.assertEQ(a.foo(TestEnum1.A), "invoke1");

    let b: Test2 = new Test2();
    arktest.assertEQ(b.foo("123", TestEnum1.A), "invoke1");
    arktest.assertEQ(b.foo(123, TestEnum1.A), "invoke3");
    arktest.assertEQ(b.foo("123", TestEnum2.C), "invoke3");
    arktest.assertEQ(b.foo(123, TestEnum2.C), "invoke2");

    let c: Test3 = new Test3();
    arktest.assertEQ(c.foo(TestEnum1.A), "invoke2");
    arktest.assertEQ(c.foo(TestEnum2.B), "invoke1");
    arktest.assertEQ(c.foo(TestEnum1.A.valueOf()), "invoke2");
    arktest.assertEQ(c.foo(TestEnum2.B.valueOf()), "invoke1");
}

