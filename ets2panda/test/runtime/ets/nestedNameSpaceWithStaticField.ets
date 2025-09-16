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

namespace A {
    export class C1{
        static arr1 = [1, 2, 3]
        arr2 = [1, 2, 3]
    }
}

namespace B {
    export namespace X {
        export let c1 = new A.C1()
        export class C2 {
            static a = 5
        }
    }
}

namespace C {
    export let a: A.C1[] = [new A.C1()]
}

C.a[0].arr2[2] *= B.X.c1.arr2[1]
arktest.assertTrue(C.a[0].arr2[2] == 6)
C.a[0].arr2[2] += A.C1.arr1[1]
arktest.assertTrue(C.a[0].arr2[2] == 8)
A.C1.arr1[1] -= B.X.C2.a + 5
arktest.assertTrue(A.C1.arr1[1] == -8)
