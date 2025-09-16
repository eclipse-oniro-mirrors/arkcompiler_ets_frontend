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

import {launch} from "std/concurrency"
import {Job} from "std/core"

type Cb = (p: int) => int;

let fs: Cb[] = []

function foo(i: int): Cb {
    return fs[i]
}

function main() {
    fs = [
        (p: int): int => p + 1,
        (p: int): int => p + 2,
        (p: int): int => p + 3,
        (p: int): int => p + 4,
        (p: int): int => p + 5,
        (p: int): int => p + 6,
        (p: int): int => p + 7,
        (p: int): int => p + 8,
        (p: int): int => p + 9,
        (p: int): int => p + 0,
    ]

    // array of promises
    let ps: Object[] = new Object[10]
    for (let i = 0; i < 10; i++) {
        ps[i] = launch<Cb, (i: int) => Cb>(foo, i)
    }

    let cnt = 0
    for (let i = 9; i >= 0; i--) {
        cnt += ((ps[i] as Job<Cb>).Await())(i) // <- complains on this line
    }
    arktest.assertEQ(cnt, 90);
}
