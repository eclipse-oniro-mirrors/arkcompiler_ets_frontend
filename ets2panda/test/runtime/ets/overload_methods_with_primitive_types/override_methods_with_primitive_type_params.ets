/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Test cases for bridge methods with primitive type parameters
// Base: primitive types (int, string)
// Derived: Object/Any (contravariance)
// Bridge needed to call Derived method through Base reference

// ============================================================================
// Test 1: Basic primitive-to-Object override (byte)
// ============================================================================
class BaseByte {
    foo(a: byte): string { return "base" }
}

class DerivedByte extends BaseByte {
    foo(a: Object): string { return "derived-byte" }
}

// ============================================================================
// Test 2: short
// ============================================================================
class BaseShort {
    foo(a: short): string { return "base" }
}

class DerivedShort extends BaseShort {
    foo(a: Object): string { return "derived-short" }
}

// ============================================================================
// Test 3: int
// ============================================================================
class BaseInt {
    foo(a: int): string { return "base" }
}

class DerivedInt extends BaseInt {
    foo(a: Object): string { return "derived-int" }
}

// ============================================================================
// Test 4: long
// ============================================================================
class BaseLong {
    foo(a: long): string { return "base" }
}

class DerivedLong extends BaseLong {
    foo(a: Object): string { return "derived-long" }
}

// ============================================================================
// Test 5: float
// ============================================================================
class BaseFloat {
    foo(a: float): string { return "base" }
}

class DerivedFloat extends BaseFloat {
    foo(a: Object): string { return "derived-float" }
}

// ============================================================================
// Test 6: double
// ============================================================================
class BaseDouble {
    foo(a: double): string { return "base" }
}

class DerivedDouble extends BaseDouble {
    foo(a: Object): string { return "derived-double" }
}

// ============================================================================
// Test 7: char
// ============================================================================
class BaseChar {
    foo(a: char): string { return "base" }
}

class DerivedChar extends BaseChar {
    foo(a: Object): string { return "derived-char" }
}

// ============================================================================
// Test 8: boolean
// ============================================================================
class BaseBoolean {
    foo(a: boolean): string { return "base" }
}

class DerivedBoolean extends BaseBoolean {
    foo(a: Object): string { return "derived-boolean" }
}

// ============================================================================
// Test 9: string
// ============================================================================
class BaseString {
    foo(a: string): string { return "base" }
}

class DerivedString extends BaseString {
    foo(a: Object): string { return "derived-string" }
}

// ============================================================================
// Test 10: All primitives with Any
// ============================================================================
class BaseAnyByte {
    foo(a: byte): string { return "base" }
}

class DerivedAnyByte extends BaseAnyByte {
    foo(a: Any): string { return "derived-any-byte" }
}

class BaseAnyShort {
    foo(a: short): string { return "base" }
}

class DerivedAnyShort extends BaseAnyShort {
    foo(a: Any): string { return "derived-any-short" }
}

class BaseAnyInt {
    foo(a: int): string { return "base" }
}

class DerivedAnyInt extends BaseAnyInt {
    foo(a: Any): string { return "derived-any-int" }
}

class BaseAnyLong {
    foo(a: long): string { return "base" }
}

class DerivedAnyLong extends BaseAnyLong {
    foo(a: Any): string { return "derived-any-long" }
}

class BaseAnyFloat {
    foo(a: float): string { return "base" }
}

class DerivedAnyFloat extends BaseAnyFloat {
    foo(a: Any): string { return "derived-any-float" }
}

class BaseAnyDouble {
    foo(a: double): string { return "base" }
}

class DerivedAnyDouble extends BaseAnyDouble {
    foo(a: Any): string { return "derived-any-double" }
}

class BaseAnyChar {
    foo(a: char): string { return "base" }
}

class DerivedAnyChar extends BaseAnyChar {
    foo(a: Any): string { return "derived-any-char" }
}

class BaseAnyBoolean {
    foo(a: boolean): string { return "base" }
}

class DerivedAnyBoolean extends BaseAnyBoolean {
    foo(a: Any): string { return "derived-any-boolean" }
}

class BaseAnyString {
    foo(a: string): string { return "base" }
}

class DerivedAnyString extends BaseAnyString {
    foo(a: Any): string { return "derived-any-string" }
}

// ============================================================================
// Test 11: Method with multiple parameters - different primitives
// ============================================================================
class BaseMultiParam {
    method(a: int, b: string): string { return "base" }
}

class DerivedMultiParam extends BaseMultiParam {
    method(a: Object, b: Object): string { return "derived-multi" }
}

// ============================================================================
// Test 12: Overloaded methods with primitive types
// ============================================================================
class BaseOverloadIntInt {
    foo(x: int, y: int): string { return "base-overload-1" }
    foo(x: Object, y: int): string { return "base-overload-2" }
}

class DerivedOverloadIntInt extends BaseOverloadIntInt {
    override foo(x: Object, z: Object): string { return "derived-overload" }
}

function main() {
    let ts = new arktest.ArkTestsuite("override_methods_with_primitive_type_params")

    // Test 1: byte
    ts.addTest("byte to Object", () => {
        let derived = new DerivedByte()
        let base: BaseByte = derived as BaseByte
        arktest.assertEQ(base.foo(1), "derived-byte")
    })

    // Test 2: short
    ts.addTest("short to Object", () => {
        let derived = new DerivedShort()
        let base: BaseShort = derived as BaseShort
        arktest.assertEQ(base.foo(1), "derived-short")
    })

    // Test 3: int
    ts.addTest("int to Object", () => {
        let derived = new DerivedInt()
        let base: BaseInt = derived as BaseInt
        arktest.assertEQ(base.foo(1), "derived-int")
    })

    // Test 4: long
    ts.addTest("long to Object", () => {
        let derived = new DerivedLong()
        let base: BaseLong = derived as BaseLong
        arktest.assertEQ(base.foo(1), "derived-long")
    })

    // Test 5: float
    ts.addTest("float to Object", () => {
        let derived = new DerivedFloat()
        let base: BaseFloat = derived as BaseFloat
        arktest.assertEQ(base.foo(1.0), "derived-float")
    })

    // Test 6: double
    ts.addTest("double to Object", () => {
        let derived = new DerivedDouble()
        let base: BaseDouble = derived as BaseDouble
        arktest.assertEQ(base.foo(1.0), "derived-double")
    })

    // Test 7: char
    ts.addTest("char to Object", () => {
        let derived = new DerivedChar()
        let base: BaseChar = derived as BaseChar
        let c: char = c'a'
        arktest.assertEQ(base.foo(c), "derived-char")
    })

    // Test 8: boolean
    ts.addTest("boolean to Object", () => {
        let derived = new DerivedBoolean()
        let base: BaseBoolean = derived as BaseBoolean
        arktest.assertEQ(base.foo(true), "derived-boolean")
    })

    // Test 9: string
    ts.addTest("string to Object", () => {
        let derived = new DerivedString()
        let base: BaseString = derived as BaseString
        arktest.assertEQ(base.foo("test"), "derived-string")
    })

    // Test 10: All primitives to Any
    ts.addTest("byte to Any", () => {
        let derived = new DerivedAnyByte()
        let base: BaseAnyByte = derived as BaseAnyByte
        arktest.assertEQ(base.foo(1), "derived-any-byte")
    })

    ts.addTest("short to Any", () => {
        let derived = new DerivedAnyShort()
        let base: BaseAnyShort = derived as BaseAnyShort
        arktest.assertEQ(base.foo(1), "derived-any-short")
    })

    ts.addTest("int to Any", () => {
        let derived = new DerivedAnyInt()
        let base: BaseAnyInt = derived as BaseAnyInt
        arktest.assertEQ(base.foo(1), "derived-any-int")
    })

    ts.addTest("long to Any", () => {
        let derived = new DerivedAnyLong()
        let base: BaseAnyLong = derived as BaseAnyLong
        arktest.assertEQ(base.foo(1), "derived-any-long")
    })

    ts.addTest("float to Any", () => {
        let derived = new DerivedAnyFloat()
        let base: BaseAnyFloat = derived as BaseAnyFloat
        arktest.assertEQ(base.foo(1.0), "derived-any-float")
    })

    ts.addTest("double to Any", () => {
        let derived = new DerivedAnyDouble()
        let base: BaseAnyDouble = derived as BaseAnyDouble
        arktest.assertEQ(base.foo(1.0), "derived-any-double")
    })

    ts.addTest("char to Any", () => {
        let derived = new DerivedAnyChar()
        let base: BaseAnyChar = derived as BaseAnyChar
        let c: char = c'a'
        arktest.assertEQ(base.foo(c), "derived-any-char")
    })

    ts.addTest("boolean to Any", () => {
        let derived = new DerivedAnyBoolean()
        let base: BaseAnyBoolean = derived as BaseAnyBoolean
        arktest.assertEQ(base.foo(true), "derived-any-boolean")
    })

    ts.addTest("string to Any", () => {
        let derived = new DerivedAnyString()
        let base: BaseAnyString = derived as BaseAnyString
        arktest.assertEQ(base.foo("test"), "derived-any-string")
    })

    // Test 11: Method with multiple parameters
    ts.addTest("Multi param - int, string to Object", () => {
        let derived = new DerivedMultiParam()
        let base: BaseMultiParam = derived as BaseMultiParam
        arktest.assertEQ(base.method(1, "test"), "derived-multi")
    })

    // Test 12: Overloaded methods with primitive types
    ts.addTest("Overloaded methods - int,int and Object,int to Object,Object", () => {
        let derived = new DerivedOverloadIntInt()
        let base: BaseOverloadIntInt = derived as BaseOverloadIntInt
        arktest.assertEQ(base.foo("aa", 0), "derived-overload")
    })

    return ts.run()
}
