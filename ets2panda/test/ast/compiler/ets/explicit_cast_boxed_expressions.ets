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

let testByte: Byte = new Byte(42 as byte);
let testShort: Short = new Short(42 as short);
let testInt: Int = new Int(42 as int);
let testLong: Long = new Long(42 as long);
let testFloat: Float = new Float(42 as float);
let testDouble: Double = new Double(42 as double);
let testChar: Char = new Char(Int.toChar(42));
let testLongValue: Long = 9223372036854775807;

function byte_test(): boolean {
    let Byte_: Byte = new Byte(42 as byte); // ?

    let byte_short  = Byte.toShort(Byte_); // ?
    let byte_int    = Byte.toInt(Byte_); // ?
    let byte_long   = Byte.toLong(Byte_); // ok
    let byte_float  = Byte.toFloat(Byte_); // ok
    let byte_double = Byte.toDouble(Byte_); // ok
    let byte_char   = Byte.toChar(Byte_); // ok

    // true test Type speciefic operations
    {
        if (byte_double != testDouble || byte_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (byte_char != testChar || byte_char.isBinDigit() != testChar.isBinDigit()) {
            return false;
        }
        if (byte_float != testFloat || byte_float.isNaN() != testFloat.isNaN()) {
            return false;
        }
        if (byte_long != testLong || (byte_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }

        // no int test
        // no short test
        // no byte test
    }
    return true;
}
function short_test(): boolean {
    let Short_: Short = new Short(42 as short);


    let short_byte   = Short.toByte(Short_);
    let short_short  = Short.toShort(Short_);
    let short_char   = Short.toChar(Short_);
    let short_int    = Short.toInt(Short_);
    let short_long   = Short.toLong(Short_);
    let short_float  = Short.toFloat(Short_);
    let short_double = Short.toDouble(Short_);


    // true test Type speciefic operations
    {
        if (short_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (short_char != testChar || short_char.isBinDigit() != testChar.isBinDigit()) {
            return false;
        }
        if (short_float != testFloat || short_float.isNaN() != testFloat.isNaN()) {
            return false;
        }
        if (short_long != testLong || (short_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function char_test(): boolean {
    let Char_: Char = new Char(Int.toChar(42));

    let char_byte   = Char.toByte(Char_);
    let char_short  = Char.toShort(Char_);
    let char_char   = Char.toChar(Char_);
    let char_int    = Char.toInt(Char_);
    let char_long   = Char.toLong(Char_);
    let char_float  = Char.toFloat(Char_);
    let char_double = Char.toDouble(Char_);

    // true test Type speciefic operations
    {
        if (char_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (char_char != testChar || char_char.isBinDigit() != testChar.isBinDigit()) {
            return false;
        }
        if (char_float != testFloat || char_float.isNaN() != testFloat.isNaN()) {
            return false;
        }
        if (char_long != testLong || (char_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function int_test(): boolean {
    let Int_: Int = new Int(42 as int);


    let int_byte    = Int.toByte(Int_);
    let int_short   = Int.toShort(Int_);
    let int_char    = Int.toChar(Int_);
    let int_int     = Int.toInt(Int_);
    let int_long    = Int.toLong(Int_);
    let int_float   = Int.toFloat(Int_);
    let int_double  = Int.toDouble(Int_);

    // true test Type speciefic operations
    {
        if (int_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (int_char != testChar || int_char.isBinDigit() != testChar.isBinDigit()) {
            return false;
        }
        if (int_float != testFloat || int_float.isNaN() != testFloat.isNaN()) {
            return false;
        }
        if (int_long != testLong || (int_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function long_test(): boolean {
    let Long_: Long = new Long(42 as long);

    let long_byte   = Long.toByte(Long_);
    let long_short  = Long.toShort(Long_);
    let long_char   = Long.toChar(Long_);
    let long_int    = Long.toInt(Long_);
    let long_long   = Long.toLong(Long_);
    let long_float  = Long.toFloat(Long_);
    let long_double = Long.toDouble(Long_);

    // true test Type speciefic operations
    {
        if (long_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (long_char != testChar || long_char.isBinDigit() != testChar.isBinDigit()) {
            return false;
        }
        if (long_float != testFloat || long_float.isNaN() != testFloat.isNaN()) {
            return false;
        }
        if (long_long != testLong || (long_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function float_test(): boolean {
    let Float_: Float = new Float(42 as float);

    let float_byte   = Float.toByte(Float_);
    let float_short  = Float.toShort(Float_);
    let float_int    = Float.toInt(Float_);
    let float_long   = Float.toLong(Float_);
    let float_float  = Float.toFloat(Float_);
    let float_double = Float.toDouble(Float_);
    // true test Type speciefic operations
    {
        if (float_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (float_float != testFloat || float_float.isNaN() != testFloat.isNaN()) { // better to find another way of checking
            return false;
        }
        if (float_long != testLong || (float_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function double_test(): boolean {
    let Double_: Double = new Double(42 as double);

    let double_byte   = Double.toByte(Double_);
    let double_short  = Double.toShort(Double_);
    let double_int    = Double.toInt(Double_);
    let double_long   = Double.toLong(Double_);
    let double_float  = Double.toFloat(Double_);
    let double_double = Double.toDouble(Double_);
    // true test Type speciefic operations
    {
        if (double_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        if (double_float != testFloat || double_float.isNaN() != testFloat.isNaN()) { // better to find another way of checking
            return false;
        }
        if (double_long != testLong || (double_long = testLongValue) != (testLongValue)) {
            return false; // in fact CTE
        }
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function boolean_test(): boolean {
    let boolean_: boolean = true;
    let Boolean_: Boolean = new Boolean(true);

    let boolean_boolean = Boolean_ as Boolean;
    return true;
}

function main(): int {
    if (byte_test() && short_test() && char_test() && int_test()
        && long_test() && float_test() && double_test() && boolean_test()) {
        return 0;
    }
    return 1;
}
