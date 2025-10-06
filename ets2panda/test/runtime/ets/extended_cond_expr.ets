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

enum E {zero, first, second, third}
class C {}

let arr = [555, 666]
let c = new C()

// 'A && B' => A is true, so the result should have type of B
arktest.assertEQ(arr["a" && 1], 666)
arktest.assertEQ(arr[true && 1], 666)
arktest.assertEQ(arr[1.0 && 1], 666)
arktest.assertEQ(arr[8237549235487923 && 1], 666)

// 'A && B' => A is false, so the result should have type of A
arktest.assertEQ(arr[0 && "a"], 555)
arktest.assertEQ(arr[0 && true], 555)
arktest.assertEQ(arr[0 && 1.0], 555)
arktest.assertEQ(arr[0 && 8237549235487923], 555)

// 'A || B' => A is true, so the result should have type of A
arktest.assertEQ(arr[1 || ""], 666)
arktest.assertEQ(arr[1 || false], 666)
arktest.assertEQ(arr[1 || 0.0], 666)
arktest.assertEQ(arr[1 || 0], 666)
arktest.assertEQ(arr[1 || 8237549235487923], 666)

// 'A || B' => A is false, so the result should have type of B
arktest.assertEQ(arr["" || 0], 555)
arktest.assertEQ(arr[false || 0], 555)
arktest.assertEQ(arr[0.0 || 0], 555)
arktest.assertEQ(arr[0 || 0], 555)
