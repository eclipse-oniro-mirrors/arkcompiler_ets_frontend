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

function main(): void {
  let a_int : int = 2147;
  let b_Num : Number = Int.toDouble(a_int);

  arktest.assertTrue(b_Num instanceof Number && b_Num.unboxed() == 2147.0)

  let a_Short : Short = 215;
  let b_double : double = Short.toDouble(a_Short);

  arktest.assertTrue(a_Short instanceof Short)
  arktest.assertEQ(b_double, 215.0)

  let a_char : char = 128 as char;
  let b_Char = a_char as Char;

  arktest.assertEQ(b_Char.unboxed(), 128 as char)

  return;
}