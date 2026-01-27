/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

type UnionTypeUndefined = int | undefined
type UnionTypeUndefinedAndNull = int | undefined | null | string

function main(): int {
    let v: UnionTypeUndefined[] = new UnionTypeUndefined[33]
    for (let i = 0; i < 33; i++) {
      arktest.assertTrue(v[i] instanceof undefined, "v[i] value has to be undefined.")
    }

    let y: UnionTypeUndefinedAndNull[] = new UnionTypeUndefinedAndNull[33]
    for (let i = 0; i < 33; i++) {
      arktest.assertTrue(y[i] instanceof undefined, "y[i] value has to be undefined.")
    }

    let m : FixedArray<int> = new FixedArray<int>(33)
    for (let i = 0; i < 33; i++) {
      arktest.assertEQ(m[i], 0, "m[i] value has to be 0.")
    }

    return 0
}
