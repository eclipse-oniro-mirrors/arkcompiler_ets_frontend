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

const constchar: char = c'p'

function main() {
    const constcharToByte = constchar as byte
    arktest.assertEQ(constcharToByte ,112)
    enum TestEnum1 { One = constcharToByte }
    arktest.assertEQ(TestEnum1.One.valueOf(),112)

    const constcharToShort = constchar as short
    arktest.assertEQ(constcharToShort ,112)
    enum TestEnum2 { One = constcharToShort }
    arktest.assertEQ(TestEnum2.One.valueOf(),112)

    const constcharToInt = constchar as int
    arktest.assertEQ(constcharToInt ,112)
    enum TestEnum3 { One = constcharToInt }
    arktest.assertEQ(TestEnum3.One.valueOf(),112)

    const constcharToLong = constchar as long
    arktest.assertEQ(constcharToLong ,112)
    enum TestEnum4 { One = constcharToLong }
    arktest.assertEQ(TestEnum4.One.valueOf(),112)
}


/* @@? 19:29 Error TypeError: Cannot cast type 'Char' to 'Byte'  */
/* @@? 21:5 Error SyntaxError: Illegal start of ENUM expression.  */
/* @@? 22:22 Error TypeError: Unresolved reference TestEnum1  */
/* @@? 24:30 Error TypeError: Cannot cast type 'Char' to 'Short'  */
/* @@? 26:5 Error SyntaxError: Illegal start of ENUM expression.  */
/* @@? 27:22 Error TypeError: Unresolved reference TestEnum2  */
/* @@? 29:28 Error TypeError: Cannot cast type 'Char' to 'Int'  */
/* @@? 31:5 Error SyntaxError: Illegal start of ENUM expression.  */
/* @@? 32:22 Error TypeError: Unresolved reference TestEnum3  */
/* @@? 34:29 Error TypeError: Cannot cast type 'Char' to 'Long'  */
/* @@? 36:5 Error SyntaxError: Illegal start of ENUM expression.  */
/* @@? 37:22 Error TypeError: Unresolved reference TestEnum4  */
