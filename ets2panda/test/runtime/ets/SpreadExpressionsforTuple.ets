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


function main() {
    let x1: [int, string] = [1, "a"]
    let y1: [int, string] = [...x1]
    arktest.assertTrue(y1[0] == 1 && y1[1] == "a")

    let x2: [int, boolean] = [2, true]
    let y2: [string, int, boolean, int, boolean, string] = ["a", ...x2, ...x2, "b"]
    arktest.assertTrue(y2[0] == "a" && y2[1] == 2 && y2[2] == true)
    arktest.assertTrue(y2[3] == 2 && y2[4] == true && y2[5] == "b")

    let x3: [int, boolean] = [10, false]
    let x4: [string] = ["e"]
    let x5: [string, boolean] = ["f", true]
    let y3: [int, boolean, string, string, int, string, boolean] = [...x3, "g", ...x4, 20, ...x5]
    arktest.assertTrue(y3[0] == 10 && y3[1] == false && y3[2] == "g")
    arktest.assertTrue(y3[3] == "e" && y3[4] == 20 && y3[5] == "f" && y3[6] == true)
}
