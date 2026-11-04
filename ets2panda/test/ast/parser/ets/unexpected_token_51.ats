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

function identity<Type>(arg: Type): Type {
  return arg;
}
let output = identity<string>
let output1 = identity<number>
console.log(output("hehehe"))
/* @@ label1 */console.log(/* @@ label2 */output(/* @@ label3 */4))

/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Boolean): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Byte): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Short): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Char): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Int): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Long): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Float): void` */
/* @@@ label1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Double): void` */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for (Int) */
/* @@@ label3 Error Semantic error ESE0046: Type 'Int' is not compatible with type 'String' at index 1 */
