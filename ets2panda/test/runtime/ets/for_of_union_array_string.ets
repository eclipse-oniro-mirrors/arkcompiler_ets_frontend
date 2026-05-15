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

type ArrayOrString = FixedArray<int> | ValueArray<int> | string

function collect(input: ArrayOrString): string {
    let result = ""
    for (let value: string | int of input) {
        if (value instanceof string) {
            result += value
        } else {
            result += value.toString()
        }
    }
    return result
}

function testFixedArray() {
    let arr: FixedArray<int> = [1, 2, 3]
    arktest.assertEQ(collect(arr), "123")
}

function testValueArray() {
    let arr: ValueArray<int> = [4, 5, 6]
    arktest.assertEQ(collect(arr), "456")
}

function testString() {
    arktest.assertEQ(collect("abc"), "abc")
}

function testBreakAndContinue() {
    // Wrap input into a lambda to avoid smart casting
    let input: ArrayOrString = (():ArrayOrString => [7, 8, 9, 10] as FixedArray<int>)()
    let result = ""
    for (let value: string | int of input) {
        if (value instanceof int && value == 8) {
            continue
        }
        if (value instanceof int && value == 10) {
            break
        }
        result += value.toString()
    }
    arktest.assertEQ(result, "79")
}

function testExistingVariableIteration() {
    let input: ArrayOrString = ((): ArrayOrString => [8, 9] as FixedArray<int>)()
    let value: string | int = 0
    let result = ""
    for (value of input) {
        result += value.toString()
    }
    arktest.assertEQ(result, "89")
    arktest.assertEQ(value, 9)
}

function main() {
    testFixedArray()
    testValueArray()
    testString()
    testBreakAndContinue()
    testExistingVariableIteration()
}
