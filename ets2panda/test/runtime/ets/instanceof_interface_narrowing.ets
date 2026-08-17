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

interface IBase {
    getVal(): int
}

interface IUnrelated {}

class Base {
    field: int = 0
}

class Derived extends Base implements IBase {
    constructor(v: int) {
        super()
        this.field = v
    }
    getVal(): int {
        return this.field
    }
}

class Other {}

// Case 1: interface variable holding a Derived instance, instanceof a Base class
function testInterfaceToClass(): int {
    let i: IBase = new Derived(42)
    let b: Base
    try {
        if (i instanceof Base) {
            b = i as Base
            return b.field
        }
    } catch (e) {
        return -1
    }
    return -2
}

// Case 2: interface variable, instanceof an unrelated interface
function testInterfaceToInterface(): int {
    let i: IBase = new Derived(10)
    try {
        if (i instanceof IUnrelated) {
            return -1
        }
        return i.getVal()
    } catch (e) {
        return -2
    }
}

// Case 3: class variable, instanceof an interface
function testClassToInterface(): int {
    let b: Base = new Derived(99)
    try {
        if (b instanceof IBase) {
            let i: IBase = b as IBase
            return i.getVal()
        }
    } catch (e) {
        return -1
    }
    return -2
}

// Case 4: interface variable, instanceof truly unrelated class
function testInterfaceToUnrelatedClass(): int {
    let i: IBase = new Derived(5)
    if (i instanceof Other) {
        return -1
    }
    return i.getVal()
}

// Case 5: Diamond-like — two interfaces, one class implementing both
interface IA {
    a(): int
}
interface IB {
    b(): int
}
class Diamond implements IA, IB {
    a(): int { return 1 }
    b(): int { return 2 }
}

function testDiamondInterfaceNarrowing(): int {
    let x: IA = new Diamond()
    try {
        if (x instanceof IB) {
            let y: IB = x as IB
            return y.b()
        }
    } catch (e) {
        return -1
    }
    return -2
}

// Case 6: instanceof in else-if chain with interface narrowing
function testElseIfChain(): int {
    let x: Object = new Derived(77)
    if (x instanceof IBase) {
        let i: IBase = x as IBase
        return i.getVal()
    } else if (x instanceof Other) {
        return -1
    }
    return -2
}

function main(): void {
    arktest.assertEQ(testInterfaceToClass(), 42)
    arktest.assertEQ(testInterfaceToInterface(), 10)
    arktest.assertEQ(testClassToInterface(), 99)
    arktest.assertEQ(testInterfaceToUnrelatedClass(), 5)
    arktest.assertEQ(testDiamondInterfaceNarrowing(), 2)
    arktest.assertEQ(testElseIfChain(), 77)
}
