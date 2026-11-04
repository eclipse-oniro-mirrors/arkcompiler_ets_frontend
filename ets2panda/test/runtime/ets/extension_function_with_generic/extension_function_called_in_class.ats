/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function foo<T>(this:A<T>, p: T): T {
    return this.data;
}

class A<T> {
    data: T
    constructor(p: T) {
        this.data = p;
    }
    goo1(): T {
        return this.foo(this.data);
    }

    goo2(): T {
        return foo(this, this.data);
    }

}

function main() {
    let a1 = new A<int>(2);
    arktest.assertEQ(a1.foo(1), 2)
    arktest.assertEQ(foo(a1, 1), 2)
    arktest.assertEQ(a1.goo1(), 2)
    arktest.assertEQ(a1.goo2(), 2)
}