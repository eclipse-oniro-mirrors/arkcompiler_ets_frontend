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
  let c2 : char = c'\u0002';
  let c5 : char = c'\u0005';
  let c7 : char = c'\u0007';

  arktest.assertEQ(c'\u0002' + c'\u0005', c'\u0007')
  arktest.assertEQ(c2 + c5, c7)

  arktest.assertEQ(c'\u0007' - c'\u0005', c'\u0002')
  arktest.assertEQ(c7 - c5, c2)

  arktest.assertTrue(c'\u0002' < c'\u0005')
  arktest.assertTrue(c2 < c5)

  // u0002 -> 00000002 , -00000002 == FFFFFFFE == -2;
  arktest.assertEQ(-c'\u0002', -2)
  arktest.assertEQ(-c2, -2)

  // u0002 -> 00000002 , ~00000002 == FFFFFFFD == -3;
  arktest.assertEQ(~c'\u0002', -3)
  arktest.assertEQ(~c2, -3)

  arktest.assertEQ(c'\u0005' / c'\u0002', 2)
  arktest.assertEQ(c5 / c2, 2)

  arktest.assertEQ(c'\u0005' % c'\u0002', 1)
  arktest.assertEQ(c5 % c2, 1)

  arktest.assertEQ(c'\u0005' << c'\u0002', 20)
  arktest.assertEQ(c5 << c2, 20)

  let arr : char[] = new char[1];
  arr[0] = c2;
  arktest.assertTrue(c2 == arr[0] && arr[0] == c2)

  return;
}
