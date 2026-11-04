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

function main(): int
{
  let a_int : int = 10;
  let a_long : long = 1234;

  a_int += a_long;
  arktest.assertEQ(a_int, 1244)

  a_int = 10 as int;
  a_long = 1234 as long;
  a_int -= a_long;
  arktest.assertEQ(a_int, -1224)

  a_int = 10 as int;
  a_long = 1234 as long;
  a_int *= a_long;
  arktest.assertEQ(a_int, 12340)

  a_int = 2000 as int;
  a_long = 1234 as long;
  a_int /= a_long;
  arktest.assertEQ(a_int, 1)

  a_int = 2000 as int;
  a_long = 1234 as long;
  a_int %= a_long;
  arktest.assertEQ(a_int, 766)

  a_int = 2 as int;
  a_long = 1 as long;
  a_int ^= a_long;
  arktest.assertEQ(a_int, 3)

  a_int = 2 as int;
  a_long = 1 as long;
  a_int |= a_long;
  arktest.assertEQ(a_int, 3)

  a_int = 2 as int;
  a_long = 1 as long;
  a_int &= a_long;
  arktest.assertEQ(a_int, 0)

  a_int = 512 as int;
  a_long = 1 as long;
  a_int <<= a_long;
  arktest.assertEQ(a_int, 1024)

  a_int = 512 as int;
  a_long = 1 as long;
  a_int >>= a_long;
  arktest.assertEQ(a_int, 256)

  a_int = 512 as int;
  a_long = 1 as long;
  a_int >>>= a_long;
  arktest.assertEQ(a_int, 256)

  //----------------------

  a_int = 10 as int;
  a_long = 20000000000 as long;
  a_int += a_long;
  arktest.assertEQ(a_int, -1474836470)

  a_int = 10 as int;
  a_long = 20000000000 as long;
  a_int -= a_long;
  arktest.assertEQ(a_int, 1474836490)

  a_int = 10 as int;
  a_long = 20000000000 as long;
  a_int *= a_long;
  arktest.assertEQ(a_int, -1863462912)

  a_int = 2000 as int;
  a_long = 20000000000 as long;
  a_int /= a_long;
  arktest.assertEQ(a_int, 0)

  a_int = 2000 as int;
  a_long = 20000000000 as long;
  a_int %= a_long;
  arktest.assertEQ(a_int, 2000)

  a_int = 2 as int;
  a_long = 20000000000 as long;
  a_int ^= a_long;
  arktest.assertEQ(a_int, -1474836478)

  a_int = 2 as int;
  a_long = 20000000000 as long;
  a_int |= a_long;
  arktest.assertEQ(a_int, -1474836478)

  a_int = 2 as int;
  a_long = 20000000000 as long;
  a_int &= a_long;
  arktest.assertEQ(a_int, 0)

  a_int = 512 as int;
  a_long = 20000000000 as long;
  a_int <<= a_long;
  arktest.assertEQ(a_int, 512)

  a_int = 512 as int;
  a_long = 20000000000 as long;
  a_int >>= a_long;
  arktest.assertEQ(a_int, 512)

  a_int = 512 as int;
  a_long = 20000000000 as long;
  a_int >>>= a_long;
  arktest.assertEQ(a_int, 512)

  //----------------------

  let a_Int = 10 as Int;
  let a_Long = 20000000000 as Long;
  a_Long += a_Int;
  arktest.assertEQ(a_Long, 20000000010)

  a_Int = 10 as Int;
  a_Long = 20000000000 as Long;
  a_Long -= a_Int;
  arktest.assertEQ(a_Long, 19999999990)

  a_Int = 10 as Int;
  a_Long = 20000000000 as Long;
  a_Long *= a_Int;
  arktest.assertEQ(a_Long, 200000000000)

  a_Int = 2000 as Int;
  a_Long = 20000000000 as Long;
  a_Long /= a_Int;
  arktest.assertEQ(a_Long, 10000000)

  a_Int = 2000 as Int;
  a_Long = 20000000000 as Long;
  a_Long %= a_Int;
  arktest.assertEQ(a_Long, 0)

  a_Int = 2 as Int;
  a_Long = 20000000000 as Long;
  a_Long ^= a_Int;
  arktest.assertEQ(a_Long, 20000000002)

  a_Int = 2 as Int;
  a_Long = 20000000000 as Long;
  a_Long |= a_Int;
  arktest.assertEQ(a_Long, 20000000002)

  a_Int = 2 as Int;
  a_Long = 20000000000 as Long;
  a_Long &= a_Int;
  arktest.assertEQ(a_Long, 0)

  a_Int = 512 as Int;
  a_Long = 20000000000 as Long;
  a_Long <<= a_Int;
  arktest.assertEQ(a_Long, 20000000000)

  a_Int = 512 as Int;
  a_Long = 20000000000 as Long;
  a_Long >>= a_Int;
  arktest.assertEQ(a_Long, 20000000000)

  a_Int = 512 as Int;
  a_Long = 20000000000 as Long;
  a_Long >>>= a_Int;
  arktest.assertEQ(a_Long, 20000000000)

  return 0;
}
