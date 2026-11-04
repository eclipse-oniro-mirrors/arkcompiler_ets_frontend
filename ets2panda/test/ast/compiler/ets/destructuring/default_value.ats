/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function test_array_default() {
  let a: number, b: number, c: number;
  [a, b, c = 42] = [1, 2];
}

function test_tuple_default() {
  let x: number, y: string;
  let tuple: [number, string] = [1, "a"];
  [x = 42, y] = tuple;
}

function main() {
  test_array_default();
  test_tuple_default();
}

/* @@? 18:10 Error Syntax error ESY125256: Default is not supported in destructuring. */
/* @@? 18:20 Error Syntax error ESY82935: Initializer of length '2' provides no value for every element in destructuring of length '3'. */
/* @@? 24:4 Error Syntax error ESY125256: Default is not supported in destructuring. */
