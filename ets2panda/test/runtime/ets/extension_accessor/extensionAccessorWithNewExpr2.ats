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

    name_:string = "11";
}

get field(this: A): Int { return A.field_ }
set field(this: A, v: int) { A.field_ = v }
get bfield(this: A): boolean { return A.bfield_; }
set bfield(this: A, b: boolean) { A.bfield_ = b; }

get name(this:A): string{ return this.name_ }
set name(this:A, n: string) { this.name_ = n }
function main(): void {
    A.field_ = 30;
    A.bfield_ = true;
    // left side
    new A().field += 10;
    arktest.assertEQ(new A().field, 40)

    new A().field -= 200;
    arktest.assertEQ(new A().field, -160)

    new A().field += 260;
    new A().field *= 10;
    arktest.assertEQ(new A().field, 1000)

    new A().field /= 100;
    arktest.assertEQ(new A().field, 10)

    new A().field = 16;
    new A().field >>= 2
    arktest.assertEQ(new A().field, 4)

    new A().field = 16;
    new A().field >>>= 1
    arktest.assertEQ(new A().field, 8)

    new A().field = 8;
    new A().field <<= 1
    arktest.assertEQ(new A().field, 16)

    new A().field = 5;
    new A().field %= 2
    arktest.assertEQ(new A().field, 1)

    new A().field = 15;
    new A().field &= 7
    arktest.assertEQ(new A().field, 7)

    new A().field = 15;
    new A().field |= 7
    arktest.assertEQ(new A().field, 15)

    new A().field = 15;
    new A().field ^= 7
    arktest.assertEQ(new A().field, 8)

    // left and right side
    new A().field = 100;
    new A().field += new A().field;
    arktest.assertEQ(new A().field, 200)

    new A().field -= new A().field;
    arktest.assertEQ(new A().field, 0)

    new A().field += 100;
    new A().field *= new A().field;
    arktest.assertEQ(new A().field, 10000)

    new A().field /= new A().field;
    arktest.assertEQ(new A().field, 1)

    new A().field |= new A().field
    arktest.assertEQ(new A().field, 1)

    // right side

    let value = 100;
    new A().field = 2;

    value += new A().field;
    arktest.assertEQ(value, 102)

    value -= new A().field;
    arktest.assertEQ(value, 100)

    value *= new A().field;
    arktest.assertEQ(value, 200)

    value /= new A().field;
    arktest.assertEQ(value, 100)

    value = 16;
    value >>= new A().field
    arktest.assertEQ(value, 4)

    value = 16;
    value >>>= new A().field
    arktest.assertEQ(value, 4)

    value = 8;
    value <<= new A().field
    arktest.assertEQ(value, 32)

    value = 5;
    value %= new A().field
    arktest.assertEQ(value, 1)

    value = 15;
    value &= new A().field
    arktest.assertEQ(value, 2)

    value = 15;
    value |= new A().field
    arktest.assertEQ(value, 15)

    value = 15;
    value ^= new A().field
    arktest.assertEQ(value, 13)
}
