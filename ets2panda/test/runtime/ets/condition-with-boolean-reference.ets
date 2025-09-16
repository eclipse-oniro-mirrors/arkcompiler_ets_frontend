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

function main() : void {
  let cond: boolean = true;

  // Check conditional expression
  let a: String = "foo";
  let b: String = "bar";
  let c: String = cond ? a : b;
  arktest.assertEQ(c, "foo")

  // Check arktest.assertTrue(statement)
  arktest.assertTrue(cond)

  // Check if statement
  if (cond) {
    arktest.assertEQ(c, "foo")
  }

  // Check for statement
  let i = 0;
  for (i; cond;) {
    i += 3;
    break;
  }
  arktest.assertEQ(i, 3)

  // Check while statement
  let while_cond = true
  while (while_cond) {
    while_cond = !cond;
  }
  arktest.assertEQ(while_cond, false)

  // Check do-while statement
  do {
    arktest.assertEQ(while_cond, false)
  } while (while_cond);
}
