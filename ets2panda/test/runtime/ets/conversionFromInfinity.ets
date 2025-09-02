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


function main(): void {

let nan_byte = Double.toByte(Infinity * 0)
let nan_short = Double.toShort(Infinity * 0)
let nan_long = Double.toLong(Infinity * 0)
let nan_int = Double.toInt(Infinity * 0)
let nan_float = Double.toFloat(Infinity * 0)
let nan_double = Double.toDouble(Infinity * 0)
let nan_byte2 = Double.toByte(Infinity * 0)

arktest.assertEQ(nan_byte, 0)
arktest.assertEQ(nan_int, 0)
arktest.assertEQ(nan_short, 0)
arktest.assertEQ(nan_long, 0)
arktest.assertTrue(isNaN(nan_float))
arktest.assertTrue(isNaN(nan_double))
arktest.assertEQ(nan_byte2, 0)

let b1 = Double.toByte(Infinity) // converted at compile time, as 'Infinity' is constant
let b2: double = Infinity
let b3 = Double.toByte(b2) // converted in runtime, as b2 isn't constant

arktest.assertEQ(b1, -1)
arktest.assertEQ(b3, -1)

let l1 = Double.toLong(Infinity)
let l2: double = Infinity
let l3 = Double.toLong(l2)
let l4 = Double.toLong(-Infinity)

arktest.assertEQ(l1, 9223372036854775807)
arktest.assertEQ(l3, 9223372036854775807)
arktest.assertEQ(l4, -9223372036854775808)

let i1 = Double.toInt(Infinity)
let i2: double = Infinity
let i3 = Double.toInt(i2)
let i4 = Double.toInt(-Infinity)

arktest.assertEQ(i1, 2147483647)
arktest.assertEQ(i3, 2147483647)
arktest.assertEQ(i4, -2147483648)

let s1 = Double.toShort(Infinity)
let s2: double = Infinity
let s3 = Double.toShort(s2)
let s4 = Double.toShort(-Infinity)

arktest.assertEQ(s1, -1)
arktest.assertEQ(s3, -1)
arktest.assertEQ(s4, 0)

let f1 = Double.toFloat(Infinity)
let f2: double = Infinity
let f3 = Double.toFloat(f2)
let f4 = Double.toFloat(-Infinity)

arktest.assertEQ(f1, Infinity)
arktest.assertEQ(f3, Infinity)
arktest.assertEQ(f4, -Infinity)

let d1 = Infinity as double
let d2: double = Infinity
let d3 = d2 as double
let d4 = -Infinity as double

arktest.assertEQ(d1, Infinity)
arktest.assertEQ(d3, Infinity)
arktest.assertEQ(d4, -Infinity)
}
