/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
    let x1 = [1, "a"]
    let y1: [int, string, int, string] = /* @@ label */[/* @@ label1 */...x1, /* @@ label2 */...x1]
    let x2 = [2]
    let x3 = ["abc", "abc"]
    let y2: [boolean, int, string, string] = /* @@ label3 */[true, /* @@ label4 */...x2, /* @@ label5 */...x3]
}

/* @@? 18:56 Error TypeError: Initializer has 2 elements, but tuple requires 4 */
/* @@? 18:72 Error TypeError: 'Array<Int|String>' cannot be spread in tuple. */
/* @@? 18:94 Error TypeError: 'Array<Int|String>' cannot be spread in tuple. */
/* @@? 21:61 Error TypeError: Initializer has 3 elements, but tuple requires 4 */
/* @@? 21:83 Error TypeError: 'Array<Double>' cannot be spread in tuple. */
/* @@? 21:105 Error TypeError: 'Array<String>' cannot be spread in tuple. */
