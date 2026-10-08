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

/*
 * Enum member values are read at runtime via the generated valueOf() call in
 * every integer context (direct assignment, local and top-level const chains,
 * default parameters, arithmetic) instead of being inlined as immediates, so
 * that incremental reloads that recompile only the enum-defining file pick up
 * the new values. This test guards the value correctness of all those
 * consumption forms, plus the object semantics (enum value passing, switch)
 * which were already runtime-based.
 */

enum Level {
    LOW = 1,
    HIGH = 100
}

const TOP: int = Level.LOW;

function withDefault(n: int = Level.HIGH): int {
    return n;
}

function main(): void {
    let a: int = Level.LOW;
    arktest.assertEQ(a, 1);

    const K: int = Level.HIGH;
    arktest.assertEQ(K + 1, 101);

    arktest.assertEQ(TOP, 1);

    arktest.assertEQ(withDefault(), 100);

    arktest.assertEQ(Level.LOW + Level.HIGH, 101);

    let l: Level = Level.HIGH;
    let v: int = l;
    arktest.assertEQ(v, 100);

    switch (l) {
        case Level.HIGH:
            arktest.assertTrue(true);
            break;
        default:
            arktest.assertTrue(false);
    }
}
