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
class A1 {
    fld: Int[]
    constructor(p: Int[], q: Int[]) {
        this.fld = [...p, ...q]
    }
    meth(): Int[] {
        return [10, 20, ...this.fld, 30]
    }
}

class B1 extends A1 {
    constructor(p: Int[], q: Int[]) {
        super(p, q)
    }
    meth(): Int[] {
        return [10, 20, ...this.fld, 30]
    }
}

class A2 {
    private _fld: Boolean[] = []
    constructor(p: Boolean[], q: Boolean[]) {
        this.fld = [...p, ...q]
    }
    meth() : Boolean[] {
        return [true, ...this.fld, false]
    }
    get fld(): Boolean[]{
        return this._fld;
    }
    set fld(v: Boolean[]) {
        this._fld = v;
    }
}

class B2 extends A2 {
    constructor(p: Boolean[], q: Boolean[]) {
        super(p, q)
    }
    meth(): Boolean[] {
        return [true, ...super.fld, false]
    }
}

function main() {
    let a1: A1 = new A1([1, 2], [3])
    let arr1 = [...a1.meth()]
    arktest.assertEQ(arr1.length, 6)
    arktest.assertTrue(arr1[0] == 10 && arr1[1] == 20 && arr1[2] == 1)
    arktest.assertTrue(arr1[3] == 2 && arr1[4] == 3 && arr1[5] == 30)

    let b1: B1 = new B1([1, 2], [3])
    let arr2 = [...b1.meth()]
    arktest.assertEQ(arr2.length, 6)
    arktest.assertTrue(arr2[0] == 10 && arr2[1] == 20 && arr2[2] == 1)
    arktest.assertTrue(arr2[3] == 2 && arr2[4] == 3 && arr2[5] == 30)

    let a2: A2 = new A2([false], [true, false])
    let arr3: Boolean[] = [...a2.meth()]
    arktest.assertEQ(arr3.length, 5)
    arktest.assertTrue(arr3[0] == true && arr3[1] == false && arr3[2] == true)
    arktest.assertTrue(arr3[3] == false && arr3[4] == false)

    let b2: B2 = new B2([false], [true, false])
    let arr4: Boolean[] = [...b2.meth()]
    arktest.assertEQ(arr4.length, 5)
    arktest.assertTrue(arr4[0] == true && arr4[1] == false && arr4[2] == true)
    arktest.assertTrue(arr4[3] == false && arr4[4] == false)

}