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

final class A {
    state: int

    constructor(...items: FixedArray<int>) {
        this.state = 0
    }

    constructor(...items: FixedArray<number>) {
        this.state = items.length
    }

    public st(): int {
        return this.state
    }

    public of(...items: FixedArray<int>): int {
        return 0
    }
    public of(...items: FixedArray<number>): int {
        return items.length
    }

    public static sof(...items: FixedArray<int>): int {
        return 0
    }
    public static sof(...items: FixedArray<number>): int {
        return items.length
    }
}

final class B {
    state: int

    constructor(...items: FixedArray<number>) {
        this.state = items.length
    }

    constructor(...items: FixedArray<int>) {
        this.state = 0
    }

    public st(): int {
        return this.state
    }

    public of(...items: FixedArray<number>): int {
        return items.length
    }
    public of(...items: FixedArray<int>): int {
        return 0
    }

    public static sof(...items: FixedArray<number>): int {
        return items.length
    }
    public static sof(...items: FixedArray<int>): int {
        return 0
    }
}

function main() {
    let a1: FixedArray<number> = [-1, -2, -3, -4, -5]
    let a2: FixedArray<int> = [1, 2, 3, 4, 5]
    let a: A = new A(...a1)
    let aa: A = new A(...a2)
    let b: B = new B(...a1)
    let bb: B = new B(...a2)

    arktest.assertEQ(a.st(), 5);
    arktest.assertEQ(aa.st(), 0);
    arktest.assertEQ(b.st(), 5);
    arktest.assertEQ(bb.st(), 5);

    arktest.assertEQ(a.of(...a1), 5);
    arktest.assertEQ(a.of(...a2), 0);
    arktest.assertEQ(b.of(...a1), 5);
    arktest.assertEQ(b.of(...a2), 5);

    arktest.assertEQ(A.sof(...a1), 5);
    arktest.assertEQ(A.sof(...a2), 0);
    arktest.assertEQ(B.sof(...a1), 5);
    arktest.assertEQ(B.sof(...a2), 5);
}

