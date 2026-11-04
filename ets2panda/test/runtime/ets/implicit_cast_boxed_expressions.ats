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
let testChar: Char = new Char(42 as char);
let testLongValue: Long = 9223372036854775807;

function byte_test(): boolean {

    let byte_byte   = new Byte(42 as byte);
    let byte_short  = new Short(42 as short);
    let byte_int    = new Int(42 as int);
    let byte_long   = new Long(42 as long);
    let byte_float  = new Float(42 as float);
    let byte_double = new Double(42 as double);
    let byte_char   = new Char(42 as char);

    byte_byte   = testByte; // Byte -> Byte
    byte_short  = testByte; // Byte -> Short
    byte_int    = testByte; // Byte -> Int
    byte_long   = testByte; // Byte -> Long
    byte_float  = testByte; // Byte -> Float
    byte_double = testByte; // Byte -> Double
    byte_char   = testByte; // Byte -> Char

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

    let short_byte   = new Byte(42 as byte);
    let short_short  = new Short(42 as short);
    let short_int    = new Int(42 as int);
    let short_long   = new Long(42 as long);
    let short_float  = new Float(42 as float);
    let short_double = new Double(42 as double);
    let short_char   = new Char(42 as char);

    // short_byte   = testShort; // Short -> Byte is not available according 6.5.2 widening table and CTE happens
    short_short  = testShort; // Short -> Short
    short_int    = testShort; // Short -> Int
    short_long   = testShort; // Short -> Long
    short_float  = testShort; // Short -> Float
    short_double = testShort; // Short -> Double
    // short_char   = testShort; // Short -> Char is not available according 6.5.2 widening table and CTE happens


    // true test Type speciefic operations
    {
        if (short_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        // if (short_char != testChar || short_char.isBinDigit() != testChar.isBinDigit()) {
        //     return false;
        // } // is not available according 6.5.2 widening table and CTE happens
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

    let char_byte   = new Byte(42 as byte);
    let char_short  = new Short(42 as short);
    let char_int    = new Int(42 as int);
    let char_long   = new Long(42 as long);
    let char_float  = new Float(42 as float);
    let char_double = new Double(42 as double);
    let char_char   = new Char(42 as char);

    // char_byte   = testChar; // Char -> Byte is not available according 6.5.2 widening table and CTE happens
    // char_short  = testChar; // Char -> Short is not available according 6.5.2 widening table and CTE happens
    char_int    = testChar; // Char -> Int
    char_long   = testChar; // Char -> Long
    char_float  = testChar; // Char -> Float
    char_double = testChar; // Char -> Double
    char_char   = testChar; // Char -> Char

    // true test Type speciefic operations
    {
        if (char_double.toExponential() != testDouble.toExponential()) {
            console.log("char 1")
            return false;
        }
        if (char_char != testChar || char_char.isBinDigit() != testChar.isBinDigit()) {
            console.log("char 2")
            return false;
        }
        if (char_float != testFloat || char_float.isNaN() != testFloat.isNaN()) {
            console.log("char 2")
            return false;
        }
        //let tmpLong = testLong
        if (char_long != testLong || (char_long = testLongValue) != (testLongValue)) {
            console.log(char_long)
            console.log(testLong)
            console.log("char 4")
            return false; // in fact CTE
        }
        //testLong = tmpLong
        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function int_test(): boolean {

    let int_byte   = new Byte(42 as byte);
    let int_short  = new Short(42 as short);
    let int_int    = new Int(42 as int);
    let int_long   = new Long(42 as long);
    let int_float  = new Float(42 as float);
    let int_double = new Double(42 as double);
    let int_char   = new Char(42 as char);

    // int_byte   = testInt; // Int -> Byte is not available according 6.5.2 widening table and CTE happens
    // int_short  = testInt; // Int -> Short is not available according 6.5.2 widening table and CTE happens
    int_int    = testInt; // Int -> Int
    int_long   = testInt; // Int -> Long
    int_float  = testInt; // Int -> Float
    int_double = testInt; // Int -> Double
    // int_char   = testInt; // Int -> Char is not available according 6.5.2 widening table and CTE happens

    // true test Type speciefic operations
    {
        if (int_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        // if (int_char != testChar || int_char.isBinDigit() != testChar.isBinDigit()) {
        //     return false;
        // } is not available according 6.5.2 widening table and CTE happens
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

    let long_byte   = new Byte(42 as byte);
    let long_short  = new Short(42 as short);
    let long_int    = new Int(42 as int);
    let long_long   = new Long(42 as long);
    let long_float  = new Float(42 as float);
    let long_double = new Double(42 as double);
    let long_char   = new Char(42 as char);

    // long_byte   = testLong; // Long -> Byte is not available according 6.5.2 widening table and CTE happens
    // long_short  = testLong; // Long -> Short is not available according 6.5.2 widening table and CTE happens
    // long_int    = testLong; // Long -> Int is not available according 6.5.2 widening table and CTE happens
    long_long   = testLong; // Long -> Long
    long_float  = testLong; // Long -> Float
    long_double = testLong; // Long -> Double
    // long_char   = testLong; // Long -> Char is not available according 6.5.2 widening table and CTE happens

    // true test Type speciefic operations
    {
        if (long_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        // if (long_char != testChar || long_char.isBinDigit() != testChar.isBinDigit()) {
        //     return false;
        // } // is not available according 6.5.2 widening table and CTE happens
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

    let float_byte   = new Byte(42 as byte);
    let float_short  = new Short(42 as short);
    let float_int    = new Int(42 as int);
    let float_long   = new Long(42 as long);
    let float_float  = new Float(42 as float);
    let float_double = new Double(42 as double);
    let float_char   = new Char(42 as char);

    // float_byte   = testFloat; // Float -> Byte is not available according 6.5.2 widening table and CTE happens
    // float_short  = testFloat; // Float -> Short is not available according 6.5.2 widening table and CTE happens
    // float_int    = testFloat; // Float -> Int is not available according 6.5.2 widening table and CTE happens
    // float_long   = testFloat; // Float -> Long is not available according 6.5.2 widening table and CTE happens
    float_float  = testFloat; // Float -> Float
    float_double = testFloat; // Float -> Double
    // float_char   = testFloat; // Float -> Char is not available according 6.5.2 widening table and CTE happens

    // true test Type speciefic operations
    {
        if (float_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        // if (float_char != testChar || float_char.isBinDigit() != testChar.isBinDigit()) {
        //     return false;
        // } is not available according 6.5.2 widening table and CTE happens
        if (float_float != testFloat || float_float.isNaN() != testFloat.isNaN()) { // better to find another way of checking
            return false;
        }
        // if (float_long != testLong || (float_long = testLongValue) != (testLongValue)) {
        //     return false; // in fact CTE
        // } is not available according 6.5.2 widening table and CTE happens

        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function double_test(): boolean {

    let double_byte   = new Byte(42 as byte);
    let double_short  = new Short(42 as short);
    let double_int    = new Int(42 as int);
    let double_long   = new Long(42 as long);
    let double_float  = new Float(42 as float);
    let double_double = new Double(42 as double);
    let double_char   = new Char(42 as char);

    // double_byte   = testDouble; // Double -> Byte is not available according 6.5.2 widening table and CTE happens
    // double_short  = testDouble; // Double -> Short is not available according 6.5.2 widening table and CTE happens
    // double_int    = testDouble; // Double -> Int is not available according 6.5.2 widening table and CTE happens
    // double_long   = testDouble; // Double -> Long is not available according 6.5.2 widening table and CTE happens
    // double_float  = testDouble; // Double -> Float is not available according 6.5.2 widening table and CTE happens
    double_double = testDouble; // Double -> Double
    //  double_char   = testDouble; // Double -> Char is not available according 6.5.2 widening table and CTE happens

    // true test Type speciefic operations
    {
        if (double_double.toExponential() != testDouble.toExponential()) {
            return false;
        }
        // if (double_char != testChar || double_char.isBinDigit() != testChar.isBinDigit()) {
        //     return false;
        // } // is not available according 6.5.2 widening table and CTE happens
        // if (double_float != testFloat || double_float.isNaN() != testFloat.isNaN()) { // better to find another way of checking
        //     return false;
        // } // is not available according 6.5.2 widening table and CTE happens
        // if (double_long != testLong || (double_long = testLongValue) != (testLongValue)) {
        //     return false; // in fact CTE
        // } // is not available according 6.5.2 widening table and CTE happens

        // no int test
        // no short test
        // no byte test
    }
    return true;
}

function main(): int {
    if (byte_test() && short_test() && char_test() && int_test()
        && long_test() && float_test() && double_test()) {
        return 0;
    }
    return 1;
}
