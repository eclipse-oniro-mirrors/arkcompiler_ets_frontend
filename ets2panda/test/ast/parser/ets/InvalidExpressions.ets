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

function f1(a: int, ...b: int[] = 47): bool {
    console.log(a, b...);
    let x = (x instanceof A<string);
    return x instanceof A<int>;
}

function f2(a?: int = 44): void {}

function f3(a: int = ): void;

function f4() {
    55 = int;
    69++
    --58
    78 = type
}

function f5(...b: int[] = 47, c?: int = 88): void {}

let x = (: int => { return 7; }

let x: number => 77
let x = new int(7);
let x = new int{8};

interface C {
    f(a: int = 0): void;
}

class A {
    f1(): void {
        async x = 9
    }

    f2() {
        this.99++;
    }
}

function f6(a: int = 9 @@): void;

function f7(a: (b: int = 0) => int): void {
}


/* @@? 16:35 Error Syntax error ESY0192: Rest parameter cannot have the default value. */
/* @@? 16:37 Error Syntax error ESY0131: Identifier is needed here. */
/* @@? 16:40 Error Semantic error ESE0371: Cannot find type 'bool'. */
/* @@? 17:21 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 17:21 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 17:24 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 18:14 Error Semantic error ESE0365: Variable 'x' is accessed before it's initialization. */
/* @@? 18:35 Error Syntax error ESY0228: Unexpected token, expected '>'. */
/* @@? 18:36 Error Syntax error ESY0227: Unexpected token ';'. */
/* @@? 19:5 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 19:12 Error Syntax error ESY0227: Unexpected token 'x'. */
/* @@? 22:23 Error Syntax error ESY0132: Not enable default value with default undefined. */
/* @@? 24:10 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 24:22 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 24:22 Error Syntax error ESY0128: You didn't set the value. */
/* @@? 24:23 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 27:5 Error Semantic error ESE0025: Invalid left-hand side of assignment expression */
/* @@? 27:8 Error Syntax error ESY0072: Invalid left-hand side in assignment expression. */
/* @@? 27:10 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 28:7 Error Syntax error ESY0053: Invalid left-hand side operator. */
/* @@? 30:5 Error Syntax error ESY0071: Invalid left-hand side in prefix operation. */
/* @@? 30:5 Error Semantic error ESE0025: Invalid left-hand side of assignment expression */
/* @@? 30:8 Error Syntax error ESY0072: Invalid left-hand side in assignment expression. */
/* @@? 33:27 Error Syntax error ESY0192: Rest parameter cannot have the default value. */
/* @@? 33:29 Error Syntax error ESY0131: Identifier is needed here. */
/* @@? 33:41 Error Syntax error ESY0132: Not enable default value with default undefined. */
/* @@? 35:10 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 35:12 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 35:16 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 37:5 Error Semantic error ESE0351: Variable 'x' has already been declared. */
/* @@? 37:15 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 37:18 Error Syntax error ESY0227: Unexpected token '77'. */
/* @@? 38:5 Error Semantic error ESE0351: Variable 'x' has already been declared. */
/* @@? 39:5 Error Semantic error ESE0351: Variable 'x' has already been declared. */
/* @@? 39:16 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 47:15 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@? 47:17 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 47:17 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 48:5 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 48:5 Error Syntax error ESY0230: Expected '=>', got '}'. */
/* @@? 48:5 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 50:5 Error Semantic error ESE0140: No matching call signature with trailing lambda */
/* @@? 51:13 Error Syntax error ESY0227: Unexpected token '.99'. */
/* @@? 51:16 Error Syntax error ESY0053: Invalid left-hand side operator. */
/* @@? 55:1 Error Syntax error ESY0227: Unexpected token 'function'. */
/* @@? 55:12 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 55:24 Error Syntax error ESY0010: There is no any node to insert at the placeholder position. */
/* @@? 55:24 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 55:24 Error Syntax error ESY0227: Unexpected token '@@'. */
/* @@? 55:26 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 55:27 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 55:29 Error Syntax error ESY0295: void is a predefined type, cannot be used as an identifier */
/* @@? 57:1 Error Syntax error ESY0227: Unexpected token 'function'. */
/* @@? 57:26 Error Syntax error ESY0133: Default value is allowed only for optional parameters. */
/* @@? 115:80 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */