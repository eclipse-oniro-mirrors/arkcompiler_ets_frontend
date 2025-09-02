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

const a: Record<number, string> = {
    [((): number => 2)()]: '1'
}

const b: Record<number, (x: string) => number> = {
    [(():number => +("foo"))()]: function (y: string): number {
        return y.length;
    },
    [(():number => +("bar"))()]: (y: string):number => y.length
};

/* @@? 16:35 Error TypeError: need to specify target type for class composite */
/* @@? 17:5 Error SyntaxError: Unexpected token. */
/* @@? 17:6 Error SyntaxError: Unexpected token, expected ':'. */
/* @@? 17:22 Error SyntaxError: Unexpected token. */
/* @@? 17:25 Error SyntaxError: Unexpected token ']'. */
/* @@? 17:26 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:28 Error SyntaxError: Unexpected token '1'. */
/* @@? 18:1 Error SyntaxError: Unexpected token '}'. */
/* @@? 20:50 Error TypeError: need to specify target type for class composite */
/* @@? 21:5 Error SyntaxError: Unexpected token. */
/* @@? 21:6 Error SyntaxError: Unexpected token, expected ':'. */
/* @@? 21:20 Error TypeError: Wrong operand type for unary expression */
/* @@? 21:28 Error SyntaxError: Unexpected token. */
/* @@? 21:31 Error SyntaxError: Unexpected token ']'. */
/* @@? 21:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 21:34 Error SyntaxError: Unexpected token 'function'. */
/* @@? 21:43 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 23:6 Error SyntaxError: Unexpected token ','. */
/* @@? 23:6 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 24:20 Error TypeError: Wrong operand type for unary expression */
/* @@? 24:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:32 Error TypeError: This expression is not callable. */
/* @@? 24:36 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:36 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 24:38 Error SyntaxError: Unexpected token 'string'. */
/* @@? 24:38 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 24:44 Error SyntaxError: Unexpected token ')'. */
/* @@? 24:45 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:46 Error SyntaxError: Unexpected token 'number'. */
/* @@? 24:46 Error TypeError: The type of parameter 'number' cannot be inferred */
/* @@? 24:56 Error TypeError: Unresolved reference y */
/* @@? 25:1 Error SyntaxError: Unexpected token '}'. */
