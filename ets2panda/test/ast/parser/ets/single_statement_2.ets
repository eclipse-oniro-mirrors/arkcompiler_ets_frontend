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

while(true) /* @@ label1 */const a = 1;
function main(): int {
    console.log(aaaa)
    /* @@ label2 */)
    let hehe: int = 10
}

/* @@@ label1 Error SyntaxError: Lexical declaration is not allowed in single statement context. */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Boolean): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Byte): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Short): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Char): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Int): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Long): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Float): void` */
/* @@? 18:5 Error TypeError: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Double): void` */
/* @@? 18:17 Error TypeError: Unresolved reference aaaa */
/* @@@ label2 Error SyntaxError: Unexpected token ')'. */
