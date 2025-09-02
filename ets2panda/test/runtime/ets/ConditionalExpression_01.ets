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
  let x1 = (): number | undefined => { return 5; }();
  let y1: number = !x1 ? 3 : x1;
  let z1: number = x1 ? x1 : 4;
  arktest.assertEQ(y1, 5.0)
  arktest.assertEQ(z1, 5.0)

  let x2 = (): number | undefined => { return undefined; }();
  let y2: number = x2 == undefined ? 3 : x2;
  let z2: number = x2 != undefined ? x2 : 4;
  arktest.assertEQ(y2, 3.0)
  arktest.assertEQ(z2, 4.0)

  let x3 = (): int | undefined => { return 5; }();
  let y3: int = !x3 ? Double.toInt(3.1) : x3;
  let z3: int = x3 ? x3 : Double.toInt(4.1);
  arktest.assertEQ(y3, 5)
  arktest.assertEQ(z3, 5)

  let x4 = (): int | undefined => { return undefined; }();
  let y4: int = x4 == undefined ? Double.toInt(3.1) : x4;
  let z4: int = x4 != undefined ? x4 : Double.toInt(4.1);
  arktest.assertEQ(y4, 3)
  arktest.assertEQ(z4, 4)
}
