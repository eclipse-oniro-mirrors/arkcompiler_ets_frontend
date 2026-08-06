/**
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

function testTypeofNarrowing(value: Object): void {
    let v: Object = value

    if (typeof v === "string") {
        let len: number = v.length
        arktest.assertEQ(len, 3)
    } else if (typeof v === "byte") {
        let byteValue: byte = v
        arktest.assertTrue(byteValue === (1 as byte))
    } else if (typeof v === "char") {
        let charValue: char = v
        arktest.assertTrue(charValue === c'a')
    } else if (typeof v === "short") {
        let shortValue: short = v
        arktest.assertTrue(shortValue === (2 as short))
    } else if (typeof v === "int") {
        let inted: int = v * 2
        arktest.assertEQ(inted, 6)
    } else if (typeof v === "long") {
        let longValue: long = v
        arktest.assertTrue(longValue === (4 as long))
    } else if (typeof v === "float") {
        let floatValue: float = v
        arktest.assertTrue(floatValue === 5.0f)
    } else if (typeof v === "number") {
        let doubled: double = v * 2
        arktest.assertEQ(doubled, 5)
    } else if (typeof v === "bigint") {
        let bigintValue: bigint = v
        arktest.assertEQ(bigintValue, 6n)
    } else if (typeof v === "boolean") {
        let notV: boolean = !v
        arktest.assertTrue(notV)
    }
}

function isUndefined(value: Object | undefined): boolean {
    return value === undefined
}

function testUndefinedNarrowing(value: string | undefined): void {
    if (typeof value === "undefined") {
        let undefinedValue: undefined = value
        arktest.assertTrue(isUndefined(undefinedValue))
    } else {
        let stringValue: string = value
        arktest.assertEQ(stringValue.length, 2)
    }
}

function testUnionNarrowing(value: string | int): void {
    if (typeof value === "string") {
        let stringValue: string = value
        arktest.assertEQ(stringValue.length, 5)
    } else if (typeof value === "int") {
        let intValue: int = value
        arktest.assertEQ(intValue * 2, 14)
    }
}

function testRedundantTypeofCondition(value: number | string | boolean): void {
    if (typeof value === "string") {
        arktest.assertEQ(value.length, 4)
    } else {
        let isSupportedType =
            typeof value === "number" || typeof value === "string" || typeof value === "boolean"
        arktest.assertTrue(isSupportedType)
    }
}

function genericObjectGuard<T>(values: T[], index: int): T {
    const value = values[index]
    if (!value || typeof value !== "object") {
        return value
    }
    return value
}

function genericParameterGuard<T>(value: T): T {
    if (!value || typeof value !== "object") {
        return value
    }
    return value
}

function genericObjectNarrowing<T>(value: T | string): boolean {
    if (typeof value === "object") {
        let narrowedValue: T = value
        return typeof narrowedValue === "object"
    }
    return false
}

class Marker {}

function constrainedGenericObjectNarrowing<T extends Marker>(value: T): boolean {
    if (typeof value === "object") {
        return true
    }
    let impossibleValue: never = value
    return false
}

function testObjectNarrowing(value: Object): void {
    if (typeof value === "object") {
        let objectValue: Object = value
        arktest.assertTrue(objectValue instanceof Marker)
    } else if (typeof value === "string") {
        let stringValue: string = value
        arktest.assertEQ(stringValue.length, 4)
    }
}

function main(): void {
    testTypeofNarrowing("abc")
    testTypeofNarrowing(1 as byte)
    testTypeofNarrowing(c'a')
    testTypeofNarrowing(2 as short)
    testTypeofNarrowing(3 as int)
    testTypeofNarrowing(4 as long)
    testTypeofNarrowing(5.0f)
    testTypeofNarrowing(2.5)
    testTypeofNarrowing(6n)
    testTypeofNarrowing(false)
    testUndefinedNarrowing(undefined)
    testUndefinedNarrowing("ok")
    testUnionNarrowing("union")
    testUnionNarrowing(7)
    testRedundantTypeofCondition("text")
    testRedundantTypeofCondition(8)
    testRedundantTypeofCondition(true)
    arktest.assertEQ(genericObjectGuard<string>(["value"], 0).length, 5)
    arktest.assertEQ(genericParameterGuard<string>("parameter").length, 9)
    arktest.assertTrue(genericObjectNarrowing<Marker>(new Marker()))
    arktest.assertTrue(!genericObjectNarrowing<Marker>("string"))
    arktest.assertTrue(constrainedGenericObjectNarrowing<Marker>(new Marker()))
    testObjectNarrowing(new Marker())
    testObjectNarrowing("text")
}
