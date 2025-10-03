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

 function main() {
    let x : Record<string, number> = {
        "key1" : 1,
        "key2" : 2,
        "key3" : 3
    }

    console.log(x[/* @@ label */0])
 }

/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Boolean): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Byte): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Short): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Char): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Int): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Long): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Float): void` */
/* @@? 23:5 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Double): void` */
/* @@? 23:17 Error Semantic error ESE0127: No matching indexing signature for $_get(Int) */
/* @@@ label Error Semantic error ESE0046: Type 'Int' is not compatible with type 'String' at index 1 */
/* @@@ label Error Semantic error ESE0251: Cannot find index access method with the required signature. */