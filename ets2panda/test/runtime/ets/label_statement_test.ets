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

function testNestedLabeledForOf() {
    let matrix: int[][] = [[1, 2], [3, 4], [5, 6]]
    let target: int = 3
    let foundRow: int = -1

    rowScan: for (let row of matrix) {
        colScan: for (let val of row) {
            if (val === target) {
                foundRow = row[0]
                break rowScan
            }
        }
    }

    arktest.assertEQ(foundRow, 3)
}

function main() {
    let sum: int
    let arr1: int[] = [0, 10, 20, 30, 40]
    let arr2: int[] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    loop1: for (let y of arr1) {
        loop2: for (let x of arr2) {
            if (x == 7) {
                continue loop1;
            }
            if (x % 2 == 0) {
                continue loop2;
            }
            sum += x
        }
    }
    arktest.assertEQ(sum, 45)
    testNestedLabeledForOf()
}
