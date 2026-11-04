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

function assertArrayEQ<T>(a1: readonly T[], a2: readonly T[]) {
    arktest.assertEQ(a1.length, a2.length);
    for (let i = 0; i < a1.length; ++i) {
        arktest.assertEQ(a1[i], a2[i]);
    }
}

// check assertArrayEQ
assertArrayEQ([1,2], [1,2]);
arktest.expectThrow(() => {assertArrayEQ([1],[2]);}, e=>(e instanceof AssertionError));


class A {
    arr: readonly int[]
    constructor(arr: Iterable<int>) {
        this.arr = Array.from(arr);
    }

    foo() {
        let arr = this.arr;
        // check methods from ReadonlyArray
        arktest.assertFalse(arr.keys().next().done);
        assertArrayEQ(arr.filter((v, i, a) => i == 0), [0]);
        arktest.assertEQ(arr.at(1), 1);
        assertArrayEQ(arr.concat([11],[12,13]), [0,1,2,3,4,5,6,7,8,9,11,12,13])
        arktest.assertEQ(arr.find((v,i,a)=>i==2), 2);
        arktest.assertEQ(arr.findIndex((v,i,a)=>v==3), 3);
        arktest.assertTrue(arr.includes(4));
        arktest.assertEQ(arr.indexOf(5), 5);
        arktest.assertFalse(arr.values().next().done);
        arktest.assertFalse(arr.entries().next().done);
        assertArrayEQ(arr.map<int>((v,i,ar)=>v), arr);

        // check methods from Object
        arktest.assertEQ(arr.toString(), "0,1,2,3,4,5,6,7,8,9")
        this.arr = []
    }
}

let a = new A([0,1,2,3,4,5,6,7,8,9]);
a.foo();
