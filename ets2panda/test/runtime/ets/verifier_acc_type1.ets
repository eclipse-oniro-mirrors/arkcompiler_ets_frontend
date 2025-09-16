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

abstract class A {
    fld: boolean[] = []
    constructor(p: boolean[]) {
        this.fld = p
    }
    meth(): boolean[] {
        return this.fld
    }
}
class B extends A {
    constructor(p: boolean[], q: boolean[]) {
        super([...p, ...q])
    }
    bar(): int {
        return foo(42, ...super.meth())
    }
}
function foo(a: int, ...p: boolean[]): int {
    return Double.toInt(a + p.length)
}

function main() {
    // use super expression
    let b: B = new B([false, false], [true, true])
    arktest.assertEQ(b.bar(), 46)
}