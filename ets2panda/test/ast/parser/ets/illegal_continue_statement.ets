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
    continue
    let a: number = 10
    console.log(a + 5)
}

let a2: number = 10;
function foo2() {
    continue;
    let a2: number = 10
    console.log(a + 5)
}

let a3: number = 10;
function foo3() {
    break
    let a: number = 10
    console.log(a + 5)
}

let a4: number = 10;
function foo4() {
    break;
    let a2: number = 10
    console.log(a + 5)
}

/* @@? 18:5 Error SyntaxError: Illegal continue statement. */
/* @@? 18:5 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */
/* @@? 25:5 Error SyntaxError: Illegal continue statement. */
/* @@? 25:5 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */
/* @@? 32:5 Error SyntaxError: Illegal break statement. */
/* @@? 32:5 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */
/* @@? 39:5 Error SyntaxError: Illegal break statement. */
/* @@? 39:5 Error TypeError: Control flow redirection statement can not be used out of loop or switch statement. */