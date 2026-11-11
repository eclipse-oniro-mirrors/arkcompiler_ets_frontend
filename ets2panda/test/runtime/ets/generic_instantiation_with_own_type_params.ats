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

// Explicit instantiation of a generic class with its own type parameters must
// produce a generic entity with the provided type arguments used as is (see
// 'Generic Instantiations' and 'Explicit Generic Instantiations' in the spec).

class Pair<A, B> {
    first: A;
    second: B;

    constructor(a: A, b: B) {
        this.first = a;
        this.second = b;
    }

    swap(): Pair<B, A> {
        return new Pair<B, A>(this.second, this.first);
    }
}

class Q<A, B = A> {
    x: A;
    y: B;

    constructor(a: A, b: B) {
        this.x = a;
        this.y = b;
    }
}

function main() {
    const p = new Pair<Int, String>(1, "a");
    const s = p.swap();
    arktest.assertEQ(s.first, "a");
    arktest.assertEQ(s.second, 1);

    let q1 = new Q<int, int>(1, 1);
    let q2 = new Q<int>(1, 1);
    q1 = q2 
}
