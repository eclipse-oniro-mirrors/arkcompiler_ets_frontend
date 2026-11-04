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

function main() {
    let s : Double = new Double(-1.1E-20)

    const a : Float | Double | Number = s
    arktest.assertEQ(a as Double, -1.1E-20  )

    const b : Float | Double | Number = s
    arktest.assertEQ(b, -1.1E-20)

    const c : Double = s
    arktest.assertEQ(c as Double, -1.1E-20)

    const d : double | int = -1.1E-20
    arktest.assertEQ(d as double, -1.1E-20)
    
    let e : Float | Double | Number = s
    arktest.assertEQ(e as Double, -1.1E-20)

    let f : double | int = -1.1E-20
    arktest.assertEQ(f as double, -1.1E-20)

    const g : String | Double = "Hi"
    arktest.assertEQ(g as String, "Hi")
}