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
class B {
    field: string = "B"
}

class Test {
    foo(a?: A,): string {
        return "invoke1";
    }
    foo(a?: A, b?: B): string {
        return "invoke2";
    }
    foo(a?: B, b?: A): string {
        return "invoke3";
    }
    foo(a?: B, b?: B): string {
        return "invoke4";
    }
}

function main() {
    let a: Test = new Test();
    arktest.assertEQ(a.foo(new A()), "invoke1");
    arktest.assertEQ(a.foo(new B()), "invoke3");
    arktest.assertEQ(a.foo(new A(), new B()), "invoke2");
    arktest.assertEQ(a.foo(new B(), new A()), "invoke3");
    arktest.assertEQ(a.foo(new B(), new B()), "invoke4");
}
