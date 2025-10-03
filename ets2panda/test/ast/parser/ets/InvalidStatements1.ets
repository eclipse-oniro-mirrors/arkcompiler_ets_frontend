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

export type function f(): void {
}

export type let x: number = 7

label1:
let x = 77

class A {
    let x: number = 5;
}

function f() {
    struct A {
        let x:number;
    }
    function s(): void {}
}

try
    let x: number = 89
} catch(a) {
}

try x: number;

try
    let x: number;
catch
finally {}

class A {
    let x: number = 5;
}

throw
"abc"
// there were more errors

/* @@? 22:1 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 22:5 Error Semantic error ESE0351: Variable 'x' has already been declared. */
/* @@? 25:5 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 28:1 Error Semantic error ESE0130: Function f is already declared. */
/* @@? 29:5 Error Syntax error ESY0040: Illegal start of STRUCT expression. */
/* @@? 30:9 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 32:5 Error Syntax error ESY0135: Nested functions are not allowed. */
/* @@? 36:5 Error Syntax error ESY0230: Expected '{', got 'let'. */
/* @@? 40:1 Error Syntax error ESY0039: A try statement should contain either finally clause or at least one catch clause. */
/* @@? 40:5 Error Syntax error ESY0230: Expected '{', got 'identification literal'. */
/* @@? 40:8 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 42:1 Error Syntax error ESY0039: A try statement should contain either finally clause or at least one catch clause. */
/* @@? 43:5 Error Syntax error ESY0230: Expected '{', got 'let'. */
/* @@? 45:9 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 47:1 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 48:5 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 52:1 Error Syntax error ESY0172: Illegal newline after throw. */
/* @@? 72:79 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */