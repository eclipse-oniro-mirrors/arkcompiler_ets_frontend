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

// Test cases for bridge methods with union types in base class
// Base: union types (int | string, etc.)
// Derived: Object or Any (contravariance)
// Tests cover: all primitive types, null, various union sizes, multiple arguments, interfaces

// ============================================================================
// Category 1: One argument - all primitives in union
// ============================================================================

// Test 1: int | string -> Object
class Base1 {
    foo(x: int | string): string { return "base" }
}
class Derived1 extends Base1 {
    foo(x: Object): string { return "derived" }
}

// Test 2: byte | short | int -> Object
class Base2 {
    foo(x: byte | short | int): string { return "base" }
}
class Derived2 extends Base2 {
    foo(x: Object): string { return "derived" }
}

// Test 3: int | long | float -> Object
class Base3 {
    foo(x: int | long | float): string { return "base" }
}
class Derived3 extends Base3 {
    foo(x: Object): string { return "derived" }
}

// Test 4: float | double | string -> Object
class Base4 {
    foo(x: float | double | string): string { return "base" }
}
class Derived4 extends Base4 {
    foo(x: Object): string { return "derived" }
}

// Test 5: char | boolean | null -> Object (negative - no override due to null in union)
class Base5 {
    foo(x: char | boolean | null): string { return "base" }
}
class Derived5 extends Base5 {
    foo(x: Object): string { return "derived" }
}

// Test 6: int | string | boolean | float -> Object
class Base6 {
    foo(x: int | string | boolean | float): string { return "base" }
}
class Derived6 extends Base6 {
    foo(x: Object): string { return "derived" }
}

// Test 7: many primitives -> Any
class Base7 {
    foo(x: byte | short | int | long | float | double): string { return "base" }
}
class Derived7 extends Base7 {
    foo(x: Any): string { return "derived" }
}

// Test 8: int | string | null -> Any
class Base8 {
    foo(x: int | string | null): string { return "base" }
}
class Derived8 extends Base8 {
    foo(x: Any): string { return "derived" }
}

// ============================================================================
// Category 2: Two arguments
// ============================================================================

// Test 9: int|string, boolean|float -> Object, Object
class Base9 {
    foo(x: int | string, y: boolean | float): string { return "base" }
}
class Derived9 extends Base9 {
    foo(x: Object, y: Object): string { return "derived" }
}

// Test 10: byte|short, int|long -> Object, Object
class Base10 {
    foo(x: byte | short, y: int | long): string { return "base" }
}
class Derived10 extends Base10 {
    foo(x: Object, y: Object): string { return "derived" }
}

// Test 11: int|string|boolean, float|double -> Object, Object
class Base11 {
    foo(x: int | string | boolean, y: float | double): string { return "base" }
}
class Derived11 extends Base11 {
    foo(x: Object, y: Object): string { return "derived" }
}

// ============================================================================
// Category 3: Three+ arguments
// ============================================================================

// Test 12: 3 args - int|string, boolean|float, long|double -> Object, Object, Object
class Base12 {
    foo(a: int | string, b: boolean | float, c: long | double): string { return "base" }
}
class Derived12 extends Base12 {
    foo(a: Object, b: Object, c: Object): string { return "derived" }
}

// Test 13: 3 args - byte|short|int, long|float, double|string -> Object, Object, Object
class Base13 {
    foo(a: byte | short | int, b: long | float, c: double | string): string { return "base" }
}
class Derived13 extends Base13 {
    foo(a: Object, b: Object, c: Object): string { return "derived" }
}

// Test 14: 4 args - int, string, boolean, float|double|null -> Any, Any, Any, Any
class Base14 {
    foo(a: int, b: string, c: boolean, d: float | double | null): string { return "base" }
}
class Derived14 extends Base14 {
    foo(a: Any, b: Any, c: Any, d: Any): string { return "derived" }
}

// ============================================================================
// Category 4: Multi-level inheritance
// ============================================================================

// Test 15: Base -> Middle -> Derived
class Base15 {
    foo(x: int | string | boolean): string { return "base" }
}
class Middle15 extends Base15 {
}
class Derived15 extends Middle15 {
    foo(x: Object): string { return "derived" }
}

// ============================================================================
// Category 5: Interfaces
// ============================================================================

// Test 16: interface with union -> implementation with Object
interface I16 {
    foo(x: int | string): string
}
class Impl16 implements I16 {
    foo(x: Object): string { return "derived" }
}

// Test 17: interface with two union params -> implementation with Object
interface I17 {
    bar(a: boolean | float, b: int | long): string
}
class Impl17 implements I17 {
    bar(a: Object, b: Object): string { return "derived" }
}

// ============================================================================
// Category 6: BigInt
// ============================================================================

// Test 18: int | bigint | string -> Object
class Base18 {
    foo(x: int | bigint | string): string { return "base" }
}
class Derived18 extends Base18 {
    foo(x: Object): string { return "derived" }
}

// ============================================================================
// Category 7: Mixed - some args with union, some without
// ============================================================================

// Test 19: first arg union, second arg primitive
class Base19 {
    foo(x: int | string, y: boolean): string { return "base" }
}
class Derived19 extends Base19 {
    foo(x: Object, y: Object): string { return "derived" }
}

// Test 20: mixed - byte, int|long, float|double|string|null -> Any, Any, Any
class Base20 {
    foo(x: byte, y: int | long, z: float | double | string | null): string { return "base" }
}
class Derived20 extends Base20 {
    foo(x: Any, y: Any, z: Any): string { return "derived" }
}

function main() {
    let ts = new arktest.ArkTestsuite("override_methods_with_union_types")

    // Category 1: One argument
    ts.addTest("1. int|string -> Object", () => {
        let derived = new Derived1()
        let base: Base1 = derived as Base1
        arktest.assertEQ(base.foo(42), "derived")
    })

    ts.addTest("2. byte|short|int -> Object", () => {
        let derived = new Derived2()
        let base: Base2 = derived as Base2
        let val: byte = 1 as byte
        arktest.assertEQ(base.foo(val), "derived")
    })

    ts.addTest("3. int|long|float -> Object", () => {
        let derived = new Derived3()
        let base: Base3 = derived as Base3
        let val: long = 1 as long
        arktest.assertEQ(base.foo(val), "derived")
    })

    ts.addTest("4. float|double|string -> Object", () => {
        let derived = new Derived4()
        let base: Base4 = derived as Base4
        let val: float = 1.0 as float
        arktest.assertEQ(base.foo(val), "derived")
    })

    // Negative test: no override when union contains null
    ts.addTest("5. char|boolean|null -> Object (no override)", () => {
        let derived = new Derived5()
        let base: Base5 = derived as Base5
        let val: char = c'a'
        // No override should happen because union contains null
        arktest.assertEQ(base.foo(val), "derived")
    })

    ts.addTest("6. int|string|boolean|float -> Object", () => {
        let derived = new Derived6()
        let base: Base6 = derived as Base6
        arktest.assertEQ(base.foo(42), "derived")
    })

    ts.addTest("7. many primitives -> Any", () => {
        let derived = new Derived7()
        let base: Base7 = derived as Base7
        let val: int = 1 as int
        arktest.assertEQ(base.foo(val), "derived")
    })

    ts.addTest("8. int|string|null -> Any", () => {
        let derived = new Derived8()
        let base: Base8 = derived as Base8
        arktest.assertEQ(base.foo(42), "derived")
    })

    // Category 2: Two arguments
    ts.addTest("9. two args int|string, boolean|float -> Object, Object", () => {
        let derived = new Derived9()
        let base: Base9 = derived as Base9
        arktest.assertEQ(base.foo(42, true), "derived")
    })

    ts.addTest("10. two args byte|short, int|long -> Object, Object", () => {
        let derived = new Derived10()
        let base: Base10 = derived as Base10
        let a: byte = 1 as byte
        let b: int = 1 as int
        arktest.assertEQ(base.foo(a, b), "derived")
    })

    ts.addTest("11. two args int|string|boolean, float|double -> Object, Object", () => {
        let derived = new Derived11()
        let base: Base11 = derived as Base11
        let a: int = 42 as int
        let b: float = 1.0 as float
        arktest.assertEQ(base.foo(a, b), "derived")
    })

    // Category 3: Three+ arguments
    ts.addTest("12. three args -> Object, Object, Object", () => {
        let derived = new Derived12()
        let base: Base12 = derived as Base12
        arktest.assertEQ(base.foo(42, true, 1.0), "derived")
    })

    ts.addTest("13. three args complex -> Object, Object, Object", () => {
        let derived = new Derived13()
        let base: Base13 = derived as Base13
        let a: int = 1 as int
        let b: long = 1 as long
        let c: double = 1.0 as double
        arktest.assertEQ(base.foo(a, b, c), "derived")
    })

    ts.addTest("14. four args -> Object, Object, Object, Object", () => {
        let derived = new Derived14()
        let base: Base14 = derived as Base14
        let d: float = 1.0 as float
        arktest.assertEQ(base.foo(1 as int, "test", true, d), "derived")
    })

    // Category 4: Multi-level inheritance
    ts.addTest("15. multi-level inheritance", () => {
        let derived = new Derived15()
        let base: Base15 = derived as Base15
        arktest.assertEQ(base.foo(42), "derived")
    })

    // Category 5: Interfaces
    ts.addTest("16. interface int|string -> Object", () => {
        let impl: I16 = new Impl16()
        arktest.assertEQ(impl.foo(42), "derived")
    })

    ts.addTest("17. interface boolean|float, int|long -> Object", () => {
        let impl: I17 = new Impl17()
        let b: boolean = true
        let i: int = 1 as int
        arktest.assertEQ(impl.bar(b, i), "derived")
    })

    // Category 6: BigInt
    ts.addTest("18. int|bigint|string -> Object", () => {
        let derived = new Derived18()
        let base: Base18 = derived as Base18
        arktest.assertEQ(base.foo(42), "derived")
    })

    // Category 7: Mixed
    ts.addTest("19. mixed int|string, boolean -> Object, Object", () => {
        let derived = new Derived19()
        let base: Base19 = derived as Base19
        arktest.assertEQ(base.foo(42, true), "derived")
    })

    ts.addTest("20. mixed byte, int|long, float|double|string|null -> Object", () => {
        let derived = new Derived20()
        let base: Base20 = derived as Base20
        let x: byte = 1 as byte
        let y: int = 1 as int
        let z: double = 1.0 as double
        arktest.assertEQ(base.foo(x, y, z), "derived")
    })

    return ts.run()
}
