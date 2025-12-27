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

function main() {
  let tuple: [string, int] = ["a", 1]
  let array1: (string | int)[] = [...tuple]
  arktest.assertEQ(array1[0], "a")
  arktest.assertEQ(array1[1], 1)
  array1 = [...[...tuple]]
  arktest.assertEQ(array1[0], "a")
  arktest.assertEQ(array1[1], 1)

  let array2: FixedArray<string | int> = [...tuple]
  arktest.assertEQ(array2[0], "a")
  arktest.assertEQ(array2[1], 1)
  array2 = [...[...tuple]]
  arktest.assertEQ(array2[0], "a")
  arktest.assertEQ(array2[1], 1)
}
