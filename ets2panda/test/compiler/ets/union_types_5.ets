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

class A<T extends Object> {
    union0: T;
    constructor(union0: T) {
        this.union0 = union0
    }
}
class B {
    num: int = 42
}
class C {
    num: int = 43
}

function foo(x: B|C) {
    if (x instanceof B) {
        arktest.assertEQ(x.num, 42, "Error! num field must be 42");
    } else if (x instanceof C) {
        arktest.assertEQ(x.num, 43, "Error! num field must be 43");
    } else {
        arktest.assertTrue(false, "Error! x must be instanceof B|C");
    }
}

function bar(x: A<B|C>) {
    if (x.union0 instanceof B) {
        arktest.assertEQ(x.union0.num, 42, "Error! x.union0.num field must be 42");
    } else if (x.union0 instanceof C) {
        arktest.assertEQ(x.union0.num, 43, "Error! x.union0.num field must be 43");
    } else {
        arktest.assertTrue(false, "Error! x.union0 must be instanceof B|C");
    }
}

function main(): void {
    let a: A<B|C> = new A<B|C>(new B());
    foo(a.union0);
    bar(a);
    a.union0 = new C();
    bar(a);
    foo(a.union0);
}

