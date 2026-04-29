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

function joinStrings(...items: string[]): string {
    return String.join(items, "")
}

function countValues(...items: (string | number)[]): string {
    return items.length.toString()
}

function countAnyValues(...items: string[]): int {
    return items.length
}

class StringIterable implements Iterable<string> {
    data: string[]

    constructor(data: string[]) {
        this.data = data
    }

    $_iterator(): Iterator<string> {
        return new StringIterator(this)
    }
}

class StringIterator implements Iterator<string> {
    index: int = 0
    base: StringIterable

    constructor(base: StringIterable) {
        this.base = base
    }

    next(): IteratorResult<string> {
        if (this.index >= this.base.data.length) {
            return new IteratorResult<string>(true, undefined)
        }
        return new IteratorResult<string>(false, this.base.data[this.index++])
    }
}

class StructuralStringIterable implements Iterable<string> {
    data: string[]

    constructor(data: string[]) {
        this.data = data
    }

    $_iterator(): Iterator<string> {
        return new StructuralStringIterator(this)
    }
}

class StructuralStringIterator implements Iterator<string> {
    index: int = 0
    base: StructuralStringIterable

    constructor(base: StructuralStringIterable) {
        this.base = base
    }

    next(): IteratorResult<string> {
        if (this.index >= this.base.data.length) {
            return new IteratorResult<string>(true, undefined)
        }
        return new IteratorResult<string>(false, this.base.data[this.index++])
    }
}

class GenericStructuralIterable<T> implements Iterable<T> {
    data: T[]

    constructor(data: T[]) {
        this.data = data
    }

    $_iterator(): Iterator<T> {
        return new GenericStructuralIterator<T>(this)
    }
}

class GenericStructuralIterator<T> implements Iterator<T> {
    index: int = 0
    base: GenericStructuralIterable<T>

    constructor(base: GenericStructuralIterable<T>) {
        this.base = base
    }

    next(): IteratorResult<T> {
        if (this.index >= this.base.data.length) {
            return new IteratorResult<T>(true, undefined)
        }
        return new IteratorResult<T>(false, this.base.data[this.index++])
    }
}

interface StructuralIterableInterface extends Iterable<string> {
    $_iterator(): Iterator<string>
}

class StructuralIterableInterfaceImpl implements StructuralIterableInterface {
    data: string[]

    constructor(data: string[]) {
        this.data = data
    }

    $_iterator(): Iterator<string> {
        return new StructuralIterableInterfaceIterator(this)
    }
}

class StructuralIterableInterfaceIterator implements Iterator<string> {
    index: int = 0
    base: StructuralIterableInterfaceImpl

    constructor(base: StructuralIterableInterfaceImpl) {
        this.base = base
    }

    next(): IteratorResult<string> {
        if (this.index >= this.base.data.length) {
            return new IteratorResult<string>(true, undefined)
        }
        return new IteratorResult<string>(false, this.base.data[this.index++])
    }
}

class LoggingIterable implements Iterable<string> {
    log: string[]

    constructor(log: string[]) {
        this.log = log
    }

    $_iterator(): Iterator<string> {
        this.log.push("iterator")
        return new LoggingIterator(this.log)
    }
}

class LoggingIterator implements Iterator<string> {
    index: int = 0
    log: string[]

    constructor(log: string[]) {
        this.log = log
    }

    next(): IteratorResult<string> {
        if (this.index >= 2) {
            this.log.push("nextDone")
            return new IteratorResult<string>(true, undefined)
        }

        this.log.push("next" + this.index)
        return new IteratorResult<string>(false, this.index++ == 0 ? "x" : "y")
    }
}

function mark(log: string[], value: string): string {
    log.push(value)
    return value
}

function checkStringSpread(source: string, expected: string): void {
    let copied: string[] = [...source]
    arktest.assertEQ(String.join(copied, ""), expected)
    arktest.assertEQ(joinStrings(...source), expected)
}

function checkStringAsIterableSpread(): void {
    let source: Iterable<string> = "abc"
    let copied: string[] = [...source, ...source]
    arktest.assertEQ(String.join(copied, ""), "abcabc")
    arktest.assertEQ(joinStrings(...source), "abc")
}

function checkCustomIterableSpread(): void {
    let source = new StringIterable(["x", "y"])
    let copied: string[] = [...source]
    arktest.assertEQ(String.join(copied, ""), "xy")
    arktest.assertEQ(joinStrings(...source), "xy")
}

function checkStructuralIterableSpread(): void {
    let source = new StructuralStringIterable(["s", "t"])
    let copied: string[] = [...source]
    arktest.assertEQ(String.join(copied, ""), "st")
    arktest.assertEQ(joinStrings(...source), "st")
}

function chooseStringOrArray(useString: boolean): string | string[] {
    return useString ? "uv" : ["u", "v"]
}

function checkStringOrArrayUnionSpread(): void {
    let stringSource: string | string[] = chooseStringOrArray(true)
    let copiedString: string[] = [...stringSource]
    arktest.assertEQ(String.join(copiedString, ""), "uv")
    arktest.assertEQ(joinStrings(...stringSource), "uv")

    let arraySource: string | string[] = chooseStringOrArray(false)
    let copiedArray: string[] = [...arraySource]
    arktest.assertEQ(String.join(copiedArray, ""), "uv")
    arktest.assertEQ(joinStrings(...arraySource), "uv")
}

function chooseArrayOrCustomIterable(useArray: boolean): string[] | StringIterable {
    return useArray ? ["p", "q"] : new StringIterable(["p", "q"])
}

function checkArrayOrCustomIterableUnionSpread(): void {
    let arraySource: string[] | StringIterable = chooseArrayOrCustomIterable(true)
    let copiedArray: string[] = [...arraySource]
    arktest.assertEQ(String.join(copiedArray, ""), "pq")
    arktest.assertEQ(joinStrings(...arraySource), "pq")

    let customSource: string[] | StringIterable = chooseArrayOrCustomIterable(false)
    let copiedCustom: string[] = [...customSource]
    arktest.assertEQ(String.join(copiedCustom, ""), "pq")
    arktest.assertEQ(joinStrings(...customSource), "pq")
}

function chooseArrayOrStructuralIterable(useArray: boolean): string[] | StructuralStringIterable {
    return useArray ? ["c", "d"] : new StructuralStringIterable(["c", "d"])
}

function checkArrayOrStructuralIterableUnionSpread(): void {
    let arraySource: string[] | StructuralStringIterable = chooseArrayOrStructuralIterable(true)
    let copiedArray: string[] = [...arraySource]
    arktest.assertEQ(String.join(copiedArray, ""), "cd")
    arktest.assertEQ(joinStrings(...arraySource), "cd")

    let structuralSource: string[] | StructuralStringIterable = chooseArrayOrStructuralIterable(false)
    let copiedStructural: string[] = [...structuralSource]
    arktest.assertEQ(String.join(copiedStructural, ""), "cd")
    arktest.assertEQ(joinStrings(...structuralSource), "cd")
}

function chooseGenericStructuralIterable(
    useString: boolean
): GenericStructuralIterable<string> | GenericStructuralIterable<number> {
    return useString
        ? new GenericStructuralIterable<string>(["g", "h"])
        : new GenericStructuralIterable<number>([1.0, 2.0])
}

function checkGenericStructuralIterableUnionSpread(): void {
    let stringSource: GenericStructuralIterable<string> | GenericStructuralIterable<number> =
        chooseGenericStructuralIterable(true)
    let copiedString: (string | number)[] = [...stringSource]
    arktest.assertEQ(copiedString.length, 2)
    arktest.assertEQ(countValues(...stringSource), "2")

    let numberSource: GenericStructuralIterable<string> | GenericStructuralIterable<number> =
        chooseGenericStructuralIterable(false)
    let copiedNumber: (string | number)[] = [...numberSource]
    arktest.assertEQ(copiedNumber.length, 2)
    arktest.assertEQ(countValues(...numberSource), "2")
}

function chooseStructuralIterableInterface(useArray: boolean): string[] | StructuralIterableInterface {
    return useArray ? ["i", "j"] : new StructuralIterableInterfaceImpl(["i", "j"])
}

function checkStructuralIterableInterfaceUnionSpread(): void {
    let arraySource: string[] | StructuralIterableInterface = chooseStructuralIterableInterface(true)
    let copiedArray: string[] = [...arraySource]
    arktest.assertEQ(String.join(copiedArray, ""), "ij")
    arktest.assertEQ(joinStrings(...arraySource), "ij")

    let interfaceSource: string[] | StructuralIterableInterface = chooseStructuralIterableInterface(false)
    let copiedInterface: string[] = [...interfaceSource]
    arktest.assertEQ(String.join(copiedInterface, ""), "ij")
    arktest.assertEQ(joinStrings(...interfaceSource), "ij")
}

function chooseInterfaceIterable(useString: boolean): Iterable<string> | StringIterable {
    return useString ? "rs" : new StringIterable(["r", "s"])
}

function checkInterfaceUnionSpread(): void {
    let stringSource: Iterable<string> | StringIterable = chooseInterfaceIterable(true)
    let copiedString: string[] = [...stringSource]
    arktest.assertEQ(String.join(copiedString, ""), "rs")
    arktest.assertEQ(joinStrings(...stringSource), "rs")

    let customSource: Iterable<string> | StringIterable = chooseInterfaceIterable(false)
    let copiedCustom: string[] = [...customSource]
    arktest.assertEQ(String.join(copiedCustom, ""), "rs")
    arktest.assertEQ(joinStrings(...customSource), "rs")
}

function chooseFixedArray(useString: boolean): FixedArray<string> | FixedArray<number> {
    let strings: FixedArray<string> = ["m", "n"]
    let numbers: FixedArray<number> = [1.0, 2.0]
    return useString ? strings : numbers
}

function chooseStringOrNumberArray(useString: boolean): string[] | number[] {
    return useString ? ["m", "n"] : [1.0, 2.0]
}

function checkArrayElementUnionSpread(): void {
    let stringSource: string[] | number[] = chooseStringOrNumberArray(true)
    let copiedString: (string | number)[] = [...stringSource]
    arktest.assertEQ(copiedString.length, 2)
    arktest.assertEQ(countValues(...stringSource), "2")

    let numberSource: string[] | number[] = chooseStringOrNumberArray(false)
    let copiedNumber: (string | number)[] = [...numberSource]
    arktest.assertEQ(copiedNumber.length, 2)
    arktest.assertEQ(countValues(...numberSource), "2")
}

function checkFixedArrayUnionSpread(): void {
    let stringSource: FixedArray<string> | FixedArray<number> = chooseFixedArray(true)
    let copiedString: (string | number)[] = [...stringSource]
    arktest.assertEQ(copiedString.length, 2)
    arktest.assertEQ(countValues(...stringSource), "2")

    let numberSource: FixedArray<string> | FixedArray<number> = chooseFixedArray(false)
    let copiedNumber: (string | number)[] = [...numberSource]
    arktest.assertEQ(copiedNumber.length, 2)
    arktest.assertEQ(countValues(...numberSource), "2")
}

function chooseFixedArrayOrCustomIterable(useFixed: boolean): FixedArray<string> | StringIterable {
    let fixed: FixedArray<string> = ["t", "u"]
    return useFixed ? fixed : new StringIterable(["t", "u"])
}

function checkFixedArrayOrCustomIterableUnionSpread(): void {
    let fixedSource: FixedArray<string> | StringIterable = chooseFixedArrayOrCustomIterable(true)
    let copiedFixed: string[] = [...fixedSource]
    arktest.assertEQ(String.join(copiedFixed, ""), "tu")

    let customSource: FixedArray<string> | StringIterable = chooseFixedArrayOrCustomIterable(false)
    let copiedCustom: string[] = [...customSource]
    arktest.assertEQ(String.join(copiedCustom, ""), "tu")
}

function chooseFixedArraysOrCustomIterable(kind: int): FixedArray<string> | FixedArray<number> | StringIterable {
    let strings: FixedArray<string> = ["w", "x"]
    let numbers: FixedArray<number> = [1.0, 2.0]
    if (kind == 0) {
        return strings
    }
    if (kind == 1) {
        return numbers
    }
    return new StringIterable(["w", "x"])
}

function checkFixedArraysOrCustomIterableUnionSpread(): void {
    for (let kind: int = 0; kind < 3; kind = kind + 1) {
        let source: FixedArray<string> | FixedArray<number> | StringIterable = chooseFixedArraysOrCustomIterable(kind)
        let copied: (string | number)[] = [...source]
        arktest.assertEQ(copied.length, 2)
    }
}

function chooseAnyIterable(kind: int): Object {
    let fixed: FixedArray<string> = ["a", "b"]
    if (kind == 0) {
        return fixed
    }
    if (kind == 1) {
        return "ab"
    }
    if (kind == 2) {
        let array: string[] = ["a", "b"]
        return array
    }
    return new StringIterable(["a", "b"])
}

function checkMixedIterableUnionSpread(): void {
    for (let kind: int = 0; kind < 4; kind = kind + 1) {
        let source: FixedArray<string> | string | string[] | StringIterable =
            chooseAnyIterable(kind) as FixedArray<string> | string | string[] | StringIterable
        let copied: string[] = [...source]
        arktest.assertEQ(copied.length, 2)
        arktest.assertEQ(countAnyValues(...source), 2)
    }
}

class MixedBox {
    size: int

    constructor(...items: string[]) {
        this.size = items.length
    }
}

class MixedBaseWithRest {
    size: int

    constructor(...items: string[]) {
        this.size = items.length
    }
}

class MixedDerivedWithUnionSuper extends MixedBaseWithRest {
    constructor(source: Object) {
        super(...(source as FixedArray<string> | string | string[] | StringIterable))
    }
}

class MixedThisForwarding {
    size: int

    constructor(source: Object) {
        this(...(source as FixedArray<string> | string | string[] | StringIterable))
    }

    constructor(...items: string[]) {
        this.size = items.length
    }
}

function checkMixedIterableUnionSpreadPositions(): void {
    for (let kind: int = 0; kind < 4; kind = kind + 1) {
        let source: FixedArray<string> | string | string[] | StringIterable =
            chooseAnyIterable(kind) as FixedArray<string> | string | string[] | StringIterable
        let copied: string[] = ["z", ...source, "z"]
        arktest.assertEQ(copied.length, 4)
        arktest.assertEQ(countAnyValues("z", ...source, "z"), 4)
        arktest.assertEQ(new MixedBox("z", ...source, "z").size, 4)
        arktest.assertEQ(new MixedDerivedWithUnionSuper(source).size, 2)
        arktest.assertEQ(new MixedThisForwarding(source).size, 2)
    }
}

function checkArraySpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let copied: string[] = [mark(log, "before"), ...new LoggingIterable(log), mark(log, "after")]

    arktest.assertEQ(String.join(copied, ""), "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

function checkCallSpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let copied = joinStrings(mark(log, "before"), ...new LoggingIterable(log), mark(log, "after"))

    arktest.assertEQ(copied, "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

function chooseArrayOrLoggingIterable(useArray: boolean, log: string[]): string[] | LoggingIterable {
    return useArray ? ["x", "y"] : new LoggingIterable(log)
}

function checkUnionArraySpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let source: string[] | LoggingIterable = chooseArrayOrLoggingIterable(false, log)
    let copied: string[] = [mark(log, "before"), ...source, mark(log, "after")]

    arktest.assertEQ(String.join(copied, ""), "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

function checkUnionCallSpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let source: string[] | LoggingIterable = chooseArrayOrLoggingIterable(false, log)
    let copied = joinStrings(mark(log, "before"), ...source, mark(log, "after"))

    arktest.assertEQ(copied, "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

class BaseWithRest {
    values: string[]

    constructor(...items: string[]) {
        this.values = items
    }
}

class DerivedWithIterableSuper extends BaseWithRest {
    constructor(log: string[]) {
        super(mark(log, "before"), ...new LoggingIterable(log), mark(log, "after"))
    }
}

function checkSuperSpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let derived = new DerivedWithIterableSuper(log)

    arktest.assertEQ(String.join(derived.values, ""), "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

class DerivedWithUnionSuper extends BaseWithRest {
    constructor(log: string[]) {
        super(mark(log, "before"), ...chooseArrayOrLoggingIterable(false, log), mark(log, "after"))
    }
}

function checkSuperUnionSpreadEvaluationOrder(): void {
    let log = new Array<string>()
    let derived = new DerivedWithUnionSuper(log)

    arktest.assertEQ(String.join(derived.values, ""), "beforexyafter")
    arktest.assertEQ(String.join(log, ","), "before,iterator,next0,next1,nextDone,after")
}

function main(): void {
    let source: string = "abc"
    let copied: string[] = [...source]
    arktest.assertEQ(copied.length, 3)
    arktest.assertEQ(copied[0], "a")
    arktest.assertEQ(copied[1], "b")
    arktest.assertEQ(copied[2], "c")
    arktest.assertEQ(joinStrings(...source), "abc")

    checkStringSpread("xy", "xy")
    checkStringAsIterableSpread()
    checkCustomIterableSpread()
    checkStructuralIterableSpread()
    checkStringOrArrayUnionSpread()
    checkArrayOrCustomIterableUnionSpread()
    checkArrayOrStructuralIterableUnionSpread()
    checkGenericStructuralIterableUnionSpread()
    checkStructuralIterableInterfaceUnionSpread()
    checkInterfaceUnionSpread()
    checkArrayElementUnionSpread()
    checkFixedArrayUnionSpread()
    checkFixedArrayOrCustomIterableUnionSpread()
    checkFixedArraysOrCustomIterableUnionSpread()
    checkMixedIterableUnionSpread()
    checkMixedIterableUnionSpreadPositions()
    checkArraySpreadEvaluationOrder()
    checkCallSpreadEvaluationOrder()
    checkUnionArraySpreadEvaluationOrder()
    checkUnionCallSpreadEvaluationOrder()
    checkSuperSpreadEvaluationOrder()
    checkSuperUnionSpreadEvaluationOrder()
}
