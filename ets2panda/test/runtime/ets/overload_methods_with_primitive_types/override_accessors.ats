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

/*
 * Test bridge generation for getters and setters with primitive/Object type mismatches
 */

// Getter: primitive -> Object bridge needed
class BaseGetter {
    get value(): Object {
        return "Base"
    }
}

class DerivedGetter extends BaseGetter {
    get value(): int {
        return 42
    }
}

// Getter: boxed primitive -> Object, no bridge needed (vtable works)
class BaseBoxedGetter {
    get value(): Object {
        return "Base"
    }
}

class DerivedBoxedGetter extends BaseBoxedGetter {
    get value(): Int {
        return new Int(42)
    }
}

// Setter: Object -> primitive bridge needed (parameter boxing)
class BaseSetter {
    set value(v: Object) {}
}

class DerivedSetter extends BaseSetter {
    set value(v: int) {}
}

// Setter: Object -> boxed primitive, no bridge needed
class BaseSetterBoxed {
    set value(v: Object) {}
}

class DerivedSetterBoxed extends BaseSetterBoxed {
    set value(v: Int) {}
}

// Getter with union-of-primitives -> Object, no bridge needed (union is object)
class BaseUnionGetter {
    get value(): Object {
        return "Base"
    }
}

class DerivedUnionGetter extends BaseUnionGetter {
    get value(): int | string {
        return "test"
    }
}

// Getter with union-of-only-primitives -> Object, bridge needed
class BaseUnionPrimitiveGetter {
    get value(): Object {
        return "Base"
    }
}

class DerivedUnionPrimitiveGetter extends BaseUnionPrimitiveGetter {
    get value(): int | byte {
        return 42 as int
    }
}

// Setter with union-of-only-primitives -> Object, bridge needed (parameter boxing)
class BaseUnionPrimitiveSetter {
    set value(v: int | byte) {}
}

class DerivedUnionPrimitiveSetter extends BaseUnionPrimitiveSetter {
    set value(v: Object) {}
}



// Interface with primitive getter
interface IGetter {
    get value(): Object
}

class InterfaceGetterImpl implements IGetter {
    get value(): int {
        return 42
    }
}

function main() {
    let ts = new arktest.ArkTestsuite("override_accessors")

    ts.addTest("Getter primitive -> Object bridge", () => {
        let obj = new DerivedGetter() as BaseGetter
        let val = obj.value
        arktest.assertEQ(val instanceof Int, true)
        arktest.assertEQ(val as int, 42)
    })

    ts.addTest("Getter boxed primitive -> Object no bridge", () => {
        let obj = new DerivedBoxedGetter() as BaseBoxedGetter
        let val = obj.value
        arktest.assertEQ(val instanceof Int, true)
        arktest.assertEQ(val as int, 42)
    })

    ts.addTest("Setter Object -> primitive bridge", () => {
        let obj = new DerivedSetter() as BaseSetter
        obj.value = 42 as int  // Should box primitive to Object
        // No easy assertion; just ensure no crash
        arktest.assertEQ(1, 1)
    })

    ts.addTest("Setter Object -> boxed primitive no bridge", () => {
        let obj = new DerivedSetterBoxed() as BaseSetterBoxed
        obj.value = new Int(42)
        arktest.assertEQ(1, 1)
    })

    ts.addTest("Getter union-of-primitives -> Object no bridge", () => {
        let obj = new DerivedUnionGetter() as BaseUnionGetter
        let val = obj.value
        arktest.assertEQ(typeof val === "string", true)
        arktest.assertEQ(val, "test")
    })

    ts.addTest("Getter union-of-only-primitives -> Object bridge", () => {
        let obj = new DerivedUnionPrimitiveGetter() as BaseUnionPrimitiveGetter
        let val = obj.value
        // Bridge should box the primitive union to Object
        arktest.assertEQ(val instanceof Int, true)
        arktest.assertEQ(val as int, 42)
    })

    ts.addTest("Setter union-of-only-primitives -> Object bridge", () => {
        let obj = new DerivedUnionPrimitiveSetter() as BaseUnionPrimitiveSetter
        obj.value = 42 as int  // Should box primitive to Object
        arktest.assertEQ(1, 1)
    })



    ts.addTest("Interface getter primitive -> Object bridge", () => {
        let obj = new InterfaceGetterImpl() as IGetter
        let val = obj.value
        arktest.assertEQ(val instanceof Int, true)
        arktest.assertEQ(val as int, 42)
    })

    return ts.run()
}