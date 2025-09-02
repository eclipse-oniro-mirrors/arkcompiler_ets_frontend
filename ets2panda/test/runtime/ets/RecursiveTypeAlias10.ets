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

type A<T,C,U> = T | T[] | A<C,U,T>[];

function check(a: A<string, Int, Boolean>) {
  arktest.assertEQ((a as A<Int,Boolean,string>[])[0] as Int, 1 as Int)
  arktest.assertEQ((a as A<Int,Boolean,string>[])[1] as Int, 2 as Int)
  arktest.assertEQ(((a as A<Int,Boolean,string>[])[2] as A<Boolean, string, Int>[])[0], true as Boolean)
  arktest.assertEQ(((a as A<Int,Boolean,string>[])[2] as A<Boolean, string, Int>[])[1], false as Boolean)
  arktest.assertEQ((((a as A<Int,Boolean,string>[])[2] as A<Boolean, string, Int>[])[2] as A<string, Int, Boolean>[])[0], "test")
}

function main(): int {
  let a: A<string, Int, Boolean> = "test";

  a = [1 as Int, 2 as Int, [true as Boolean, false as Boolean, [a] as A<string, Int, Boolean>[]] as A<Boolean, string, Int>[]] as A<Int, Boolean, string>[];

  check(a);

  return 0;
}