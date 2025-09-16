/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function fn(...ns: (Object|null|undefined)[]): int {
    let sum: int = 0
    for (const n of ns) {
        sum += n as Int
    }
    return sum
}

function main(): int {
    const fp: FunctionN<int> = fn
    arktest.assertEQ(fp(2, 3, 5), 10)
    const fp2: FunctionN<int> = (...vs: (Object|null|undefined)[]): int => {
        let sum: int = 0
        for (const v of vs) {
            sum += v as Int
        }
        return sum
    }
    arktest.assertEQ((fp2(7, 11, 13), 31)
    return 0
}
