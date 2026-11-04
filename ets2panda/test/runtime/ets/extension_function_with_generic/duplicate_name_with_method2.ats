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

class A<T, T1, T2> {
    data0: T;
    data1: T1;
    data2: T2;
    constructor(d0:T, d1: T1, d2: T2) {
        this.data0 = d0;
        this.data1 = d1;
        this.data2 = d2;
    }

    goo1(): T1 {
        return this.foo(this.data0);
    }

    goo2(): T2 {
        return foo(this, this.data0);
    }
    
    private foo(p: T): T1 {
        return this.data1;
    }
}

function foo<T, T1, T2>(this: A<T, T1, T2>, p: T): T2 {
    return this.data2;
}

function main() {
    let a1 = new A<String, Int, Number>("test", 1, 2.1);
    arktest.assertEQ(foo(a1, a1.data0), 2.1)
    arktest.assertEQ(a1.foo(a1.data0), 2.1)
    arktest.assertEQ(a1.goo1(), 1)
    arktest.assertEQ(a1.goo2(), 2.1)
}
