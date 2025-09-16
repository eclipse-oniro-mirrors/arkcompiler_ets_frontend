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

let count: int = 0;

function foo(a: int): char {
  count++;
  let x: char = c'd';
  return x;
}

function concatenate_float(prefix: String, number: float, suffix: String) : String {
  return (prefix + number + suffix);
}

function concatenate_compile_time() : void {
  let x: String = 5 + "10";
  arktest.assertEQ(x, "510")

  let y = 5 + "10";
  arktest.assertEQ(y, "510")
}

function main(): void {
  count = 0;
  let a: String = "abc";
  a += foo(123);

  arktest.assertEQ(a, "abcd")
  arktest.assertEQ(count, 1)

  let const_str: String = 'str' + c'a';
  arktest.assertEQ(const_str, "stra")

  arktest.assertEQ(concatenate_float('x', 1.0 as float, "y"), "x1y")

  concatenate_compile_time();
}
