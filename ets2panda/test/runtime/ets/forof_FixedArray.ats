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

enum Enum1: byte { Zero, Five = 5, Six }
enum Enum2: byte { Seven = 7, Ten = 10 }

function main() {
    let res = 0
    for (let v: Enum1|Enum2 of
        ((): FixedArray<Enum1> => [Enum1.Zero, Enum1.Five, Enum1.Six])())
        res += v.valueOf()
    arktest.assertEQ(res, 11)
}
