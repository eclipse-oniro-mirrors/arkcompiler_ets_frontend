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

function main(): void {
  let n = 2;
  let y = "d"

  let x = "abc " + y + " = " + n*2 + c'!';
  let z = `abc ${y} = ${ n*2 }!`;

  let v1 = `--- ${y + z} ---`;
  let v2 = `--- ${y + `abc ${y} = ${ n*2 }!`} ---`;

  //Console.print(x + "\n");
  //Console.print(z + "\n");
  //Console.print("'" + v1 + "'\n");
  //Console.print("'" + v2 + "'\n");

  arktest.assertEQ(x, z);
  arktest.assertEQ(v1, v2);
}
