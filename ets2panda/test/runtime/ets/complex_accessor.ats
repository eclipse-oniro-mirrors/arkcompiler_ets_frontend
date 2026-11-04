/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

let _complex: Complex = { x: 1, y: "test" }
interface Complex { x: number; y: string }
get complexObj(): Complex { return _complex }
set complexObj(v: Complex) { _complex = v }

function main() {
    // For passing JIT test purpose 
    _complex = { x: 1, y: "test" }
    arktest.assertEQ(complexObj.x, 1)
    arktest.assertEQ(complexObj.y, "test")

    complexObj = { x: 2, y: "updated" }
    arktest.assertEQ(complexObj.x, 2)
    arktest.assertEQ(complexObj.y, "updated")
}
