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

let a1: number = 10;
function foo() {
    /* @@ label1 */break
    let a: number = 10
    console.log(a + 5)
}

let a2: number = 10;
function foo2() {
    /* @@ label2 */break;
    let a2: number = 10
    console.log(a + 5)
}

/* @@@ label1 Error SyntaxError: Illegal break statement. */
/* @@@ label2 Error SyntaxError: Illegal break statement. */
/* @@? 18:20 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */
/* @@? 25:20 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */
/* @@? 27:17 Error TypeError: Unresolved reference a */
