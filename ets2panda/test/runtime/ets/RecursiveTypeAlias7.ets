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

type A<K> = string | A<K>[]

function main(): int {
  let a: A<Int> = "test";

  a = [a, [a, [a, [a] as A<Int>] as A<Int>] as A<Int>] as A<Int>;
  
  arktest.assertEQ((a as A<Int>[])[0], "test")
  arktest.assertEQ(((a as A<Int>[])[1] as A<Int>[])[0], "test")
  arktest.assertEQ((((a as A<Int>[])[1] as A<Int>[])[1] as A<Int>[])[0], "test")
  arktest.assertEQ(((((a as A<Int>[])[1] as A<Int>[])[1] as A<Int>[])[1] as A<Int>[])[0], "test")

  return 0;
}