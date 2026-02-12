/*---
Copyright (c) 2026 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
---*/

let ts = new arktest.ArkTestsuite('Limits')

function assertByte(value: byte | int) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Byte')
}

function assertShort(value: short | int) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Short')
}

function assertInt(value: int | long) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Int')
}

function assertLong(value: int | long) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Long')
}

function assertFloat(value: float | double) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Float')
}

function assertDouble(value: float | double) {
    arktest.assertEQ(Type.of(value).toString(), 'std.core.Double')
}

//  Byte

// Check for min and max values of the byte(3.6.2)
ts.addTest('Byte.MIN_VALUE') {
    assertByte(Byte.MIN_VALUE)
    arktest.assertEQ(Byte.MIN_VALUE, -128)
}
ts.addTest('Byte.MAX_VALUE') {
    assertByte(Byte.MAX_VALUE)
    arktest.assertEQ(Byte.MAX_VALUE, 127)
}
// Byte type overflow(7.24.2)
ts.addTest('Byte.Overflow') {
    arktest.assertEQ(Int.toByte(-Byte.MAX_VALUE - 1), Byte.MIN_VALUE)
    arktest.assertEQ(Int.toByte(Byte.MAX_VALUE + 1), Byte.MIN_VALUE)
    arktest.assertEQ(Int.toByte(Byte.MIN_VALUE - 1), Byte.MAX_VALUE)
    arktest.assertEQ(Byte.MIN_VALUE, Int.toByte(-Byte.MIN_VALUE))
}
// Unary minus(7.21.6)
ts.addTest('Byte.UnaryMinus') {
    assertByte(Byte.MAX_VALUE)
    assertInt(-Byte.MAX_VALUE)
}

//  Short

// Check for min and max values of the short(3.6.2)
ts.addTest('Short.MIN_VALUE') {
    assertShort(Short.MIN_VALUE)
    arktest.assertEQ(Short.MIN_VALUE, -32_768)
}
ts.addTest('Short.MAX_VALUE') {
    assertShort(Short.MAX_VALUE)
    arktest.assertEQ(Short.MAX_VALUE, 32_767)
}
// Short type overflow(7.24.2)
ts.addTest('Short.Overflow') {
    arktest.assertEQ(Int.toShort(-Short.MAX_VALUE - 1), Short.MIN_VALUE)
    arktest.assertEQ(Int.toShort(Short.MAX_VALUE + 1), Short.MIN_VALUE)
    arktest.assertEQ(Int.toShort(Short.MIN_VALUE - 1), Short.MAX_VALUE)
    arktest.assertEQ(Short.MIN_VALUE, Int.toShort(-Short.MIN_VALUE))
}
// Unary minus rules(7.21.6)
ts.addTest('Short.UnaryMinus') {
    assertShort(Short.MAX_VALUE)
    assertInt(-Short.MAX_VALUE)
}

// Int

// Check for min and max values of the integer type(3.6.2)
ts.addTest('Int.MIN_VALUE') {
    assertInt(Int.MIN_VALUE)
    arktest.assertEQ(Int.MIN_VALUE, -2_147_483_648)
}
ts.addTest('Int.MAX_VALUE') {
    assertInt(Int.MAX_VALUE)
    arktest.assertEQ(Int.MAX_VALUE, 2_147_483_647)
    arktest.assertEQ(Int.MAX_VALUE, 0x7FFF_FFFF)
}

// Integer type overflow(7.24.2)
ts.addTest('Int.Overflow') {
    arktest.assertEQ(-Int.MAX_VALUE - 1, Int.MIN_VALUE)
    arktest.assertEQ(Int.MAX_VALUE + 1, Int.MIN_VALUE)
    arktest.assertEQ(Int.MIN_VALUE - 1, Int.MAX_VALUE)
    arktest.assertEQ(Int.MIN_VALUE, -Int.MIN_VALUE)
}

// In this case, for numeric literals type is int for representing it as a positive number(2.9.2)
ts.addTest('assertInt(0x7FFF_FFFF)') {
    assertInt(0x7FFF_FFFF)
}
// Type int = operator-(int)(7.21.6)
ts.addTest('assertInt(-0x7FFF_FFFF)') {
    assertInt(-0x7FFF_FFFF)
}
ts.addTest('assertInt(-0x7FFF_FFFF-1)') {
    assertInt(-0x7FFF_FFFF-1)
}

// Long

// In this case, for numeric literals type is long for representing it as a positive number(2.9.2) 
ts.addTest('assertLong(0x8000_0000)') {
    assertLong(0x8000_0000)
}
ts.addTest('assertLong(0x8000_0013)') {
    assertLong(0x8000_0013)
}

// In this case, for numeric literals type is long for representing it as a positive number(2.9.2)
// Type long = operator-(long)(7.21.6)
// but this test fails, the type of -0x8000_0000 is int
// i think the type should be calculated like that:
// 0x8000_0000 is integer literal -> long(because it can be represented as positive number)
// -0x8000_0000 is long too unary minus in spec
ts.addTest('assertLong(-0x8000_0000)') {
    assertLong(-0x8000_0000)
}
ts.addTest('assertLong(-0x8000_0013)') {
    assertLong(-0x8000_0013)
}

// Check for min and max values of the long type(3.6.2)
ts.addTest('Long.MIN_VALUE') {
    assertLong(Long.MIN_VALUE)
    arktest.assertEQ(Long.MIN_VALUE, -9_223_372_036_854_775_808)
}
ts.addTest('Long.MAX_VALUE') {
    assertLong(Long.MAX_VALUE)
    arktest.assertEQ(Long.MAX_VALUE, 9_223_372_036_854_775_807)
    arktest.assertEQ(Long.MAX_VALUE, 0x7FFF_FFFF_FFFF_FFFF)
}

// Long type overflow(7.24.2)
ts.addTest('Long.Overflow') {
    arktest.assertEQ(-Long.MAX_VALUE - 1, Long.MIN_VALUE)
    arktest.assertEQ(Long.MAX_VALUE + 1, Long.MIN_VALUE)
    arktest.assertEQ(Long.MIN_VALUE - 1, Long.MAX_VALUE)
    arktest.assertEQ(Long.MIN_VALUE, -Long.MIN_VALUE)
}

// Check for numbers that they are in range of long(3.6.2)
ts.addTest('assertLong(0x7FFF_FFFF_FFFF_FFFF)') {
    assertLong(0x7FFF_FFFF_FFFF_FFFF)
}
ts.addTest('assertLong(-0x7FFF_FFFF_FFFF_FFFF)') {
    assertLong(-0x7FFF_FFFF_FFFF_FFFF)
}
ts.addTest('assertLong(-0x7FFF_FFFF_FFFF_FFFF-1)') {
    assertLong(-0x7FFF_FFFF_FFFF_FFFF-1)
}

// Float

// Check for min and max values of the float type(3.6.2)
ts.addTest('assertFloat(Float.MAX_VALUE)') {
    assertFloat(Float.MAX_VALUE)
}
ts.addTest('assertFloat(Float.MIN_VALUE)') {
    assertFloat(Float.MIN_VALUE)
}

// Numbers in float range(3.6.3)
ts.addTest('assertFloat(3.4e+38f)') {
    assertFloat(3.4e+38f)
}
ts.addTest('assertFloat(5e-324f)') {
    assertFloat(5e-324f)
}

// 3.6.3 Floating-Point Types and Operations
// A floating-point operation with overflow produces a signed infinity
ts.addTest('arktest.assertEQ(Float.MAX_VALUE * 2, Float.POSITIVE_INFINITY)'){
    arktest.assertEQ(Float.MAX_VALUE * 2, Float.POSITIVE_INFINITY)
}
ts.addTest('arktest.assertEQ(-Float.MAX_VALUE * 2, Float.NEGATIVE_INFINITY)'){
    arktest.assertEQ(-Float.MAX_VALUE * 2, Float.NEGATIVE_INFINITY)
}
ts.addTest('arktest.assertEQ(Float.MIN_VALUE / 2, 0)'){
    arktest.assertEQ(Float.MIN_VALUE / 2, 0)
}

// 7.22.2 Division 
// Both operands are zero
ts.addTest('arktest.assertEQ((0.0f / 0.0f).isNaN(), true)') {
    arktest.assertEQ((0.0f / 0.0f).isNaN(), true)
    assertFloat(0.0f / 0.0f)
}

// Double

// Check for min and max values of the double type(3.6.2)
ts.addTest('assertDouble(Double.MAX_VALUE)') {
    assertDouble(Double.MAX_VALUE)
}
ts.addTest('assertDouble(Double.MIN_VALUE)') {
    assertDouble(Double.MIN_VALUE)
}

// Default type for floating point literals is double(2.9.3) and numbers in double's range
ts.addTest('assertDouble(3.4e+38)') {
    assertDouble(3.4e+38)
}
ts.addTest('assertDouble(1.79e+308)') {
    assertDouble(1.79e+308)
}
ts.addTest('assertDouble(5e-324)') {
    assertDouble(5e-324)
}

// 3.6.3 Floating-Point Types and Operations
// A floating-point operation with overflow produces a signed infinity
ts.addTest('arktest.assertEQ(Double.MAX_VALUE * 2, Double.POSITIVE_INFINITY)'){
    arktest.assertEQ(Double.MAX_VALUE * 2, Double.POSITIVE_INFINITY)
}
ts.addTest('arktest.assertEQ(-Double.MAX_VALUE * 2, Double.NEGATIVE_INFINITY)'){
    arktest.assertEQ(-Double.MAX_VALUE * 2, Double.NEGATIVE_INFINITY)
}
// Underflow defined in IEEE 754
ts.addTest('arktest.assertEQ(Double.MIN_VALUE / 2, 0)'){
    arktest.assertEQ(Double.MIN_VALUE / 2, 0)
}

// 7.22.2 Division 
// Both operands are zero
// prim_boxing_nan.ets has similar check, but for 0.0f / 0.0f
ts.addTest('arktest.assertEQ((0.0 / 0.0).isNaN(), true)') {
    arktest.assertEQ((0.0 / 0.0).isNaN(), true)
    assertDouble(0.0 / 0.0)
}

// Should be CTE

// Invalid number, can't be float(it's double)
// assertFloat(1.79e+308f)

// returns that number is too large (2.9.2)
// ts.addTest('assertLong(0x8000_0000_0000_0000)') {
//     assertLong(0b1000000000000000000000000000000000000000000000000000000000000000)
//     assertLong(0o1000000000000000000000)
//     assertLong(0x8000_0000_0000_0000)
// }

// returns that number is too large (2.9.2)
// ts.addTest('assertLong(-0x8000_0000_0000_0000)') {
//     assertLong(-0b1000000000000000000000000000000000000000000000000000000000000000)
//     assertLong(-0o1000000000000000000000)
//     assertLong(-0x8000_0000_0000_0000)
// }

ts.run()
