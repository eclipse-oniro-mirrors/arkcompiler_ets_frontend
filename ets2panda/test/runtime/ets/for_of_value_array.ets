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

function testBasicIteration() {
    let arr: ValueArray<int> = [1, 2, 3, 4]
    let sum = 0
    for (let value of arr) {
        sum += value
    }
    arktest.assertEQ(sum, 10)
}

function testBreakAndContinue() {
    let arr: ValueArray<int> = [1, 2, 3, 4, 5]
    let sum = 0
    for (let value of arr) {
        if (value == 2) {
            continue
        }
        if (value == 5) {
            break
        }
        sum += value
    }
    arktest.assertEQ(sum, 8)
}

function testLabelledControlFlow() {
    let outer: ValueArray<int> = [1, 2, 3]
    let inner: ValueArray<int> = [10, 20, 30]
    let sum = 0

    loop1: for (let lhs of outer) {
        for (let rhs of inner) {
            if (rhs == 20) {
                continue loop1
            }
            if (lhs == 3) {
                break loop1
            }
            sum += lhs + rhs
        }
    }

    arktest.assertEQ(sum, 23)
}

function testConstIteration() {
    let arr: ValueArray<int> = [7, 8, 9]
    let product = 1
    for (const value of arr) {
        product *= value
    }
    arktest.assertEQ(product, 504)
}

function testExistingVariableIteration() {
    let arr: ValueArray<int> = [3, 6, 9]
    let value: int = 0
    let sum = 0
    for (value of arr) {
        sum += value
    }
    arktest.assertEQ(sum, 18)
    arktest.assertEQ(value, 9)
}

function main() {
    testBasicIteration()
    testBreakAndContinue()
    testLabelledControlFlow()
    testConstIteration()
    testExistingVariableIteration()
}
