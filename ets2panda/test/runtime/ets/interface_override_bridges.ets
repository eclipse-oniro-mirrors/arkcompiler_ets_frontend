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

// Test cases for interface bridge method generation
// Tests that bridge methods are correctly created when:
// 1. Superclass implements interface with Object/Any types
// 2. Subclass overrides with primitive types
// 3. Multiple inheritance levels with interfaces

let ts = new arktest.ArkTestsuite("")

// Test 1: Superclass implements interface, subclass overrides with primitive
interface Processor {
    process(obj: Object): void
}

class BaseProcessor implements Processor {
    process(obj: Object): void {
        // Default implementation
    }
}

class IntProcessor extends BaseProcessor {
    process(x: int): void {
        // Override with primitive type - bridge should be generated
    }
}

function testSuperclassInterfaceOverride() {
    let base: Processor = new BaseProcessor()
    let derived: Processor = new IntProcessor()

    ts.addTest("BaseProcessor through Processor interface", () => {
        base.process({})
    })
    ts.addTest("IntProcessor through Processor interface (uses bridge)", () => {
        derived.process({})
    })
}

// Test 2: Multiple inheritance levels
interface Transformer {
    transform(obj: Object): Object
}

class BaseTransformer implements Transformer {
    transform(obj: Object): Object {
        return obj
    }
}

class MiddleTransformer extends BaseTransformer {
    // Inherits transform(Object): Object
}

class IntTransformer extends MiddleTransformer {
    transform(x: int): int {
        return x
    }
}

function testMultiLevelInterface() {
    let transformer: Transformer = new IntTransformer()

    ts.addTest("IntTransformer through Transformer interface (uses bridge)", () => {
        let result = transformer.transform(42)
        arktest.assertEQ(result, 42)
    })
}

// Test 3: Multiple interfaces in hierarchy
interface Interface1 {
    method1(obj: Object): void
}

interface Interface2 {
    method2(obj: Object): void
}

class BaseClass implements Interface1, Interface2 {
    method1(obj: Object): void {}
    method2(obj: Object): void {}
}

class DerivedClass extends BaseClass {
    method1(x: int): void {}
    method2(y: int): void {}
}

function testMultipleInterfaces() {
    let derived1: Interface1 = new DerivedClass()
    let derived2: Interface2 = new DerivedClass()

    ts.addTest("DerivedClass through Interface1 (uses bridge)", () => {
        derived1.method1({})
    })
    ts.addTest("DerivedClass through Interface2 (uses bridge)", () => {
        derived2.method2({})
    })
}

// Test 4: Interface inheritance in hierarchy
interface IContainer {
    get item(): Object
}

interface IBox extends IContainer {
    // Inherits get item(): Object
}

class BaseBox implements IBox {
    private _item: Object = {}

    get item(): Object {
        return this._item
    }

    set item(v: Object) {
        this._item = v
    }
}

class MyIntBox extends BaseBox {
    private _intItem: int = 0

    get item(): int {
        return this._intItem
    }

    set item(v: int) {
        this._intItem = v
    }
}

function testInterfaceInheritanceOverride() {
    let box: IContainer = new MyIntBox()

    ts.addTest("MyIntBox through IContainer interface (uses bridge)", () => {
        let item = box.item
    })
}

// Test 5: Getter/setter from interface
interface IPropertyHolder {
    get value(): Object
    set value(v: Object)
}

class BasePropertyHolder implements IPropertyHolder {
    private _value: Object = {}

    get value(): Object {
        return this._value
    }

    set value(v: Object) {
        this._value = v
    }
}

class IntPropertyHolder extends BasePropertyHolder {
    private _intValue: int = 0

    get value(): int {
        return this._intValue
    }

    set value(v: int) {
        this._intValue = v
    }
}

function testInterfaceAccessorOverride() {
    let holder: IPropertyHolder = new IntPropertyHolder()

    ts.addTest("IntPropertyHolder getter through IPropertyHolder (uses bridge)", () => {
        let v = holder.value
    })
    ts.addTest("IntPropertyHolder setter through IPropertyHolder (uses bridge)", () => {
        holder.value = {}
    })
}

// Test 6: Any type parameters
interface AnyInterface {
    processAny(obj: Any): Any
}

class BaseAnyImpl implements AnyInterface {
    processAny(obj: Any): Any {
        return obj
    }
}

class IntAnyImpl extends BaseAnyImpl {
    processAny(x: int): int {
        return x
    }
}

function testAnyTypeOverride() {
    let impl: AnyInterface = new IntAnyImpl()

    ts.addTest("IntAnyImpl through AnyInterface (uses bridge)", () => {
        let result = impl.processAny(42)
        arktest.assertEQ(result, 42)
    })
}

// Test 7: Deep hierarchy with multiple interface implementations
interface Level1 {
    level1Method(obj: Object): void
}

interface Level2 {
    level2Method(obj: Object): void
}

class Level1Base implements Level1 {
    level1Method(obj: Object): void {}
}

class Level2Base extends Level1Base implements Level2 {
    level2Method(obj: Object): void {}
}

class Level3Derived extends Level2Base {
    level1Method(x: int): void {}
    level2Method(y: int): void {}
}

function testDeepHierarchy() {
    let derived1: Level1 = new Level3Derived()
    let derived2: Level2 = new Level3Derived()

    ts.addTest("Level3Derived through Level1 (uses bridge)", () => {
        derived1.level1Method({})
    })
    ts.addTest("Level3Derived through Level2 (uses bridge)", () => {
        derived2.level2Method({})
    })
}

// Test 8: Diamond scenario with interface
interface Root {
    rootMethod(obj: Object): void
}

interface Branch1 extends Root {
    // Inherits rootMethod
}

interface Branch2 extends Root {
    // Inherits rootMethod
}

class BaseImpl implements Branch1, Branch2 {
    rootMethod(obj: Object): void {}
}

class DerivedImpl extends BaseImpl {
    rootMethod(x: int): void {}
}

function testDiamondInterface() {
    let impl: Root = new DerivedImpl()

    ts.addTest("DerivedImpl through Root (uses bridge)", () => {
        impl.rootMethod({})
    })
}

function main() {
    testSuperclassInterfaceOverride()
    testMultiLevelInterface()
    testMultipleInterfaces()
    testInterfaceInheritanceOverride()
    testInterfaceAccessorOverride()
    testAnyTypeOverride()
    testDeepHierarchy()
    testDiamondInterface()

    return ts.run()
}
