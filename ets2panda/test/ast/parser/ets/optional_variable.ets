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


let a1?: number = 1
let a2? = 1

const b1?: string = "123"
const b2? = "123"

function main() {
   let c1?: number = 1
   let c2? = 1

   const d1?: string = "123"
   const d2? = "123"
}

/* @@? 17:8 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 18:9 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 20:10 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 21:11 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 24:11 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 25:12 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 27:13 Error SyntaxError: Optional variable is deprecated and no longer supported. */
/* @@? 28:14 Error SyntaxError: Optional variable is deprecated and no longer supported. */
