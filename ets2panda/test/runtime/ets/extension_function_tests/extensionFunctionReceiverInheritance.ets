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

class Base {}
class Fruit extends Base {}

function name(x: Base): int { return 2 }
function name(this: Fruit): int { return 1 }

function main(): int {
    let fruit = new Fruit()
    let base = new Base()

    // Method call: only extension functions participate (§15), first-match
    arktest.assertEQ(fruit.name(), 1)

    // Ordinary call: first-match, all signatures considered
    arktest.assertEQ(name(fruit), 2)

    // Ordinary call with Base: first-match
    arktest.assertEQ(name(base), 2)

    return 0
}
