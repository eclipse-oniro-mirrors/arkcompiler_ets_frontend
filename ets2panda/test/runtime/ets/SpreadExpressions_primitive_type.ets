/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

type t = (int|string|boolean)

function main() {

    let intArray: int[] = [77, 88, 99]
    let expectedIntArr: int[] = [1, 2, 77, 88, 99, 4, 6, 77, 88, 99]
    let newIntArr: int[] = [1, 2, ...intArray, 4, 6, ...intArray]
    arktest.assertEQ(expectedIntArr.length, newIntArr.length)
    for (let i = 0; i < expectedIntArr.length; i++) {
        arktest.assertEQ(expectedIntArr[i], newIntArr[i])
    }

    let strArray0: string[] = ["a", "b", "c"]
    let strArray1: string[] = ["d", "e"]
    let expectedStrArr: string[] = ["1", "2", "a", "b", "c", "3", "d", "e", "4", "5"]
    let newStrArr: string[] = ["1", "2", ...strArray0, "3", ...strArray1, "4", "5"]
    arktest.assertEQ(expectedStrArr.length, newStrArr.length)
    for (let i = 0; i < expectedStrArr.length; i++) {
        arktest.assertEQ(expectedStrArr[i], newStrArr[i])
    }

    let boolArray0: boolean[] = [true, false]
    let boolArray1: boolean[] = [true, true, false]
    let expectedBoolArr: boolean[] = [true, false, true, false, true, true, false]
    let newBoolArr: boolean[] = [...boolArray0, true, false, ...boolArray1]
    arktest.assertEQ(expectedBoolArr.length, newBoolArr.length)
    for (let i = 0; i < expectedBoolArr.length; i++) {
        arktest.assertEQ(expectedBoolArr[i], newBoolArr[i])
    }

    let tArray0: t[] = [1, "a", true]
    let tArray1: t[] = [2, "b", false]
    let newTArr: t[] = [10, "aa", ...tArray0, ...tArray1]
    let expectedTArr: t[] = [10, "aa", 1, "a", true, 2, "b", false]
    arktest.assertEQ(expectedTArr.length, newTArr.length)
    for (let i = 0; i < expectedTArr.length; i++) {
        arktest.assertEQ(expectedTArr[i], newTArr[i])
    }




}
