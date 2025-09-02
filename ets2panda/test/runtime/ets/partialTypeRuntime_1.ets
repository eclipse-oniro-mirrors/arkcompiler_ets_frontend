/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

type partial_A = Partial<A>;

class A {
    num_memb: number;
    str_memb: String = "";
    b_memb: B = new B();
}

class B { fld: number = 6 }
class C { c_num_memb: number; }
class D extends C { d_num_memb: number; }

function main(): void {
    let part_a_1: Partial<A> = {
        num_memb: 2.0,
        str_memb: "part_a_1",
        b_memb: new B()
    };

    arktest.assertEQ(part_a_1.num_memb, 2.0)
    arktest.assertEQ(part_a_1.str_memb, "part_a_1")
    arktest.assertEQ(part_a_1.b_memb?.fld, 6)
    part_a_1.num_memb = undefined
    arktest.assertEQ(part_a_1.num_memb, undefined)

    let part_a_2: Partial<A> = {
        num_memb: 4.0,
        str_memb: "part_a_2"
    };

    arktest.assertEQ(part_a_2.num_memb, 4.0)
    arktest.assertEQ(part_a_2.str_memb, "part_a_2")
    arktest.assertEQ(part_a_2.b_memb, undefined)

    let part_a_3: Partial<D> = { d_num_memb: 3.0, c_num_memb: 5.0 };
    part_a_3.d_num_memb = undefined
    arktest.assertEQ(part_a_3.c_num_memb, 5.0)
    arktest.assertEQ(part_a_3.d_num_memb, undefined)

    let part_a_4: partial_A = {
        num_memb: 6.0,
        str_memb: "part_a_4",
        b_memb: new B()
    };

    arktest.assertEQ(part_a_4.num_memb, 6.0)
    arktest.assertEQ(part_a_4.str_memb, "part_a_4")
    arktest.assertEQ(part_a_4.b_memb?.fld, 6)
    part_a_4.num_memb = undefined
    arktest.assertEQ(part_a_4.num_memb, undefined)
}
