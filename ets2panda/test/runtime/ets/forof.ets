/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

function sum1(...p: (Int)[]): int {
    let res = 0
    for (let n: Int of p) res += n ?? 0
    return res;
}
function sum2(...p: (Int | undefined)[]): int {
    let res = 0
    for (let n: Int | undefined of p) res += n ?? 0
    return res;
}
function sum3(...p: (Int | null)[]): int {
    let res = 0
    for (let n: Int | null of p) res += n ?? 0
    return res;
}

function main() {
    arktest.assertEQ(6, sum1(1, 2, 3))
    arktest.assertEQ(3, sum2(1, 2, undefined))
    arktest.assertEQ(3, sum3(1, 2, null))
}