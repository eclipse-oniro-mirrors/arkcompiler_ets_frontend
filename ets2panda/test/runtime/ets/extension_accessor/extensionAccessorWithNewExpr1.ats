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
    static field_: Int = 30;
    static bfield_: boolean = true;
}

get field(this: A): Int { return A.field_ }
set field(this: A, v: int) { A.field_ = v }
get bfield(this: A): boolean { return A.bfield_; }
set bfield(this: A, b: boolean) { A.bfield_ = b; }

function main(): void {
    A.field_ = 30;
    A.bfield_ = true;
    arktest.assertEQ(new A().field, 30)
    new A().field = 29;
    arktest.assertEQ(new A().field, 29)
    new A().field = 15 + 15;

    let z = new A().field + new A().field;
    arktest.assertEQ(z, 60)
    z = new A().field + new A().field + new A().field;
    arktest.assertEQ(z, 90)

    let u = new A().field + 10;
    arktest.assertEQ(u, 40)
    let i = 10 + new A().field;
    arktest.assertEQ(i, 40)
    u = 10 + new A().field + new A().field + 10;
    new A().field =  10 + new A().field + new A().field + 10;
    arktest.assertEQ(u, 80)
    arktest.assertEQ(new A().field, 80)

    new A().field = 10;
    let k = new A().field;
    arktest.assertEQ(k, 10)

    new A().field = new A().field;
    k = new A().field;
    arktest.assertEQ(k, 10)

    new A().field = new A().field + new A().field;
    k = new A().field;
    arktest.assertEQ(new A().field, 20)

    new A().field = new A().field + 10;
    k = new A().field;
    arktest.assertEQ(k, 30)

    new A().field = 10 + new A().field;
    k = new A().field;
    arktest.assertEQ(k, 40)

    new A().field = 10 * new A().field;
    k = new A().field;
    arktest.assertEQ(k, 400)

    new A().field = new A().field / 2;
    k = new A().field;
    arktest.assertEQ(k, 200)

    new A().field = 1;
    new A().field = new A().field & 0;
    k = new A().field;
    arktest.assertEQ(k, 0)

    new A().bfield = true;
    new A().bfield = new A().bfield & false;
    let boolk = new A().bfield;
    arktest.assertEQ(boolk, false)

    new A().bfield = true;
    new A().bfield = new A().bfield | false;
    boolk = new A().bfield;
    arktest.assertEQ(boolk, true)

    new A().bfield = true;
    new A().bfield = new A().bfield | new A().bfield;
    boolk = new A().bfield;
    arktest.assertEQ(boolk, true)
}
