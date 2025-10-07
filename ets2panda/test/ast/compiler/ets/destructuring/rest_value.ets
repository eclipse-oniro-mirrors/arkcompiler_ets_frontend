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

function test_array_rest() {
  let a: number, b: number, rest: number[];
  [a,b, ...rest] = [1,2,3,4,5,6];
}

function test_tuple_rest() {
  let x: number, y: string, rest: number[];
  let tuple: [number, string, number, number] = [1, "a", 2, 3];
  [x, y, ...rest] = tuple;
}

function main() {
  test_array_rest();
  test_tuple_rest();
}

/* @@? 18:9 Error Syntax error ESY165518: Rest is not supported in destructuring. */
/* @@? 24:10 Error Syntax error ESY165518: Rest is not supported in destructuring. */
