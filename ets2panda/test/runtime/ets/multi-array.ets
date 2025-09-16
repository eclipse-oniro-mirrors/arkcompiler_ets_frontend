/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
  const DATA : double[][] = [
    [0.1, 1.2],
    [2.3, 3.4],
    [4.5, 5.6],
    [6.7, 7.8]
  ];

  arktest.assertEQ(DATA[0][0], 0.1)
  arktest.assertEQ(DATA[0][1], 1.2)
  arktest.assertEQ(DATA[1][0], 2.3)
  arktest.assertEQ(DATA[1][1], 3.4)
  arktest.assertEQ(DATA[2][0], 4.5)
  arktest.assertEQ(DATA[2][1], 5.6)
  arktest.assertEQ(DATA[3][0], 6.7)
  arktest.assertEQ(DATA[3][1], 7.8)
  DATA[1][1] = 8.9;
  arktest.assertEQ(DATA[1][1], 8.9)

  let b: String[][] = new String[2][2];

  b[0][0] = "hello";
  arktest.assertEQ(b[0][0], "hello")

  let strArray: String[][] = [
    ["ab", "ac"],
    ["bb", "bc"]
  ];

  arktest.assertEQ(strArray[0][0], "ab")
  arktest.assertEQ(strArray[0][1], "ac")
  arktest.assertEQ(strArray[1][0], "bb")
  arktest.assertEQ(strArray[1][1], "bc")
}
