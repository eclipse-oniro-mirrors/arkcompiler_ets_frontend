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


function foo(a: int): double {
    return a.toDouble() + 10.5;
}

function main() {
    let b: byte = 1
    let c: char = c'a'
    let s: short = 5
    let i: int = 25
    let l: long = 125
    let f: float = 2.71f
    let d: double = 3.14

    // byte
    arktest.assertEQ(b.toChar(),  c'\u0001')
    arktest.assertEQ(b.toShort(), 1 as short)
    arktest.assertEQ(b.toInt(),   1 as int)
    arktest.assertEQ(b.toLong(),  1 as long)
    arktest.assertEQ(b.toFloat(), 1.0f)
    arktest.assertEQ(b.toDouble(),1.0)

    // short
    arktest.assertEQ(s.toByte(),  5 as byte)
    arktest.assertEQ(s.toChar(),  c'\u0005')
    arktest.assertEQ(s.toInt(),   5 as int)
    arktest.assertEQ(s.toLong(),  5 as long)
    arktest.assertEQ(s.toFloat(), 5.0f)
    arktest.assertEQ(s.toDouble(),5.0)

    // int
    arktest.assertEQ(i.toByte(),  25 as byte)
    arktest.assertEQ(i.toChar(),  c'\u0019')
    arktest.assertEQ(i.toShort(), 25 as short)
    arktest.assertEQ(i.toLong(),  25 as long)
    arktest.assertEQ(i.toFloat(), 25.0f)
    arktest.assertEQ(i.toDouble(),25.0)

    arktest.assertEQ(foo(i), 35.5)

    // long
    arktest.assertEQ(l.toByte(),  125 as byte)
    arktest.assertEQ(l.toChar(),  c'\u007d')
    arktest.assertEQ(l.toShort(), 125 as short)
    arktest.assertEQ(l.toInt(),   125 as int)
    arktest.assertEQ(l.toFloat(), 125.0f)
    arktest.assertEQ(l.toDouble(),125.0)

    // float
    arktest.assertEQ(f.toByte(),  2 as byte)
    arktest.assertEQ(f.toShort(), 2 as short)
    arktest.assertEQ(f.toInt(),   2 as int)
    arktest.assertEQ(f.toLong(),  2.0 as long)
    arktest.assertEQ(f.toDouble(),2.71f as double)

    // double
    arktest.assertEQ(d.toByte(),  3 as byte)
    arktest.assertEQ(d.toShort(), 3 as short)
    arktest.assertEQ(d.toInt(),   3 as int)
    arktest.assertEQ(d.toLong(),  3.0 as long)
    arktest.assertEQ(d.toFloat(), 3.14f)
}
