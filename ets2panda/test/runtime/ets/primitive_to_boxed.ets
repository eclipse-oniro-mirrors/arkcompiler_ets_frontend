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

function fooBoolean() : Boolean {
    return true;
}

function fooByte() : Byte {
    return 1;
}

function fooShort() : Short {
    return 1;
}

function fooChar() : Char {
    return c'c';
}


function fooInt() : Int {
    return 1;
}

function fooLong() : Long {
    return 1;
}

function fooFloat() : Float {
    return 1;
}

function fooDouble() : Double {
    return 1;
}


enum Color { Green = 1 , Red = 2, Blue = 3 }

function fooEnum() : Color{
    return Color.Green;
}

function main() {
    let c : Boolean = true;
    let val : Boolean = fooBoolean()
    arktest.assertEQ(val, c)

    let c1 : Byte = 1;
    let val1 : Byte = fooByte()
    arktest.assertEQ(val1, c1)

    let c2 : Short = 1;
    let val2 : Short = fooShort()
    arktest.assertEQ(val2, c2)

    let c3 : Char = c'c';
    let val3 : Char = fooChar()
    arktest.assertEQ(val3, c3)

    let c4 : Int = 1;
    let val4 : Int = fooInt()
    arktest.assertEQ(val4, c4)

    let c5 : Long = 1;
    let val5 : Long = fooLong()
    arktest.assertEQ(val5, c5)

    let c6 : Float = 1;
    let val6 : Float = fooFloat()
    arktest.assertEQ(val6, c6)


    let c7 : Double = 1;
    let val7 : Double = fooDouble()
    arktest.assertEQ(val7, c7)

    let c8 : Color = Color.Green;
    let val8 : Color = fooEnum();
    arktest.assertEQ(c8, val8)
}
