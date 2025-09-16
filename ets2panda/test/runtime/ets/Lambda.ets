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

let lambda1: () => void = (): void => {
  let obj: Object | null = null;
}

let lambda2: (value: int) => Int = (value: int): Int => {
  let a = 42;
  let num: Int = new Int(a + value);
  return num;
}

function main() {
  let local_int = 100;
  lambda1();

  arktest.assertEQ(lambda2(local_int), 142)
  local_int = 0;
  arktest.assertEQ(lambda2(local_int), 42)

  const const_int = 100;
  const const_local_int = local_int;
  let lambda3: (b: byte) => int = (b: byte): int => {
    let num: Int = new Int(const_int + const_local_int);
    let num_int: int = num;
    let result: int = num_int + b + lambda2(const_local_int);
    return result;
  }
  arktest.assertEQ(lambda3(42 as byte), 100 + 42 + 42)
}
