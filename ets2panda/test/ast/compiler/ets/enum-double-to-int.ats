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

enum Color: double {
  Red = 1.1,
  Green = 2.2,
  Blue = 3.3
}

function foo(a: int) {
	console.log(a);
}

function main(): void {
  let c1: int = Color.Blue;
  foo(Color.Blue);
  let c2: int = Color.Red + Color.Green;
}

/* @@? 27:17 Error Semantic error ESE0318: Type 'Color' cannot be assigned to type 'Int' */
/* @@? 28:3 Error Semantic error ESE0127: No matching call signature for foo(Color) */
/* @@? 28:7 Error Semantic error ESE0046: Type 'Color' is not compatible with type 'Int' at index 1 */
/* @@? 29:17 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Int' */
