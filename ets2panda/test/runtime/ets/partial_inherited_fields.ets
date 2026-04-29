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

interface I {
    i: int
}

class A implements I {
    i: int = 0
    j: String = "base"
}

class B extends A {
    k: boolean = false
}

function main(): void {
    let partial: Partial<B> = {}

    partial.i = 1
    partial.j = "derived"
    partial.k = true

    arktest.assertEQ(partial.i, 1)
    arktest.assertEQ(partial.j, "derived")
    arktest.assertEQ(partial.k, true)
}
