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
 
type readonly_A = Readonly<A>;

class A {
    num_mem: Number = 0
    str_mem: String = ""
    b_mem: B = new B()
}

class B {fld: Number = 6}

class C {c_num_mem: Number = 0}

class D  extends C {d_num_mem: Number = 0}


function main(): void {
    let readonly_a: Readonly<A> = {
        num_mem: 1,
        str_mem: "readonly_a",
        b_mem: new B()
    }
	
    arktest.assertEQ(readonly_a.num_mem, 1)
    arktest.assertEQ(readonly_a.str_mem, "readonly_a")
    arktest.assertEQ(readonly_a.b_mem.fld, 6)
	
    let readonly_d: Readonly<D> = {
        c_num_mem: 2,
        d_num_mem: 3
    }
    arktest.assertEQ(readonly_d.c_num_mem, 2)
    arktest.assertEQ(readonly_d.d_num_mem, 3)
	
    let readonly_a2: readonly_A = {
	num_mem: 4,
        str_mem: "readonly_a2",
        b_mem: new B()
    }
    arktest.assertEQ(readonly_a2.num_mem, 4)
    arktest.assertEQ(readonly_a2.str_mem, "readonly_a2")
    arktest.assertEQ(readonly_a2.b_mem.fld, 6)
	
}
