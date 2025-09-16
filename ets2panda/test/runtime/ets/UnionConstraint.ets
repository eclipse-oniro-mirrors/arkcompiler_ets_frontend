/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class X { }
function id<T extends string | number | X>(x: T) { return x }

function id_s(x: string) { return id(x) }
function id_sx(x: string | X) { return id(x) }

function main() {
    let x = new X();

    arktest.assertEQ(id(1.1), 1.1)
    arktest.assertEQ(id("a"), "a")
    arktest.assertEQ(id(x), x)

    arktest.assertEQ(id_s("a"), "a")
    arktest.assertEQ(id_sx(x), x)
}
