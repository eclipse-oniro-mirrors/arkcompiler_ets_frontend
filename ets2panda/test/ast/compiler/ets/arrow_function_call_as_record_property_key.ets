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
/* @@? 17:25 Error SyntaxError: Unexpected token ']'. */
/* @@? 17:25 Error SyntaxError: Unexpected token ']'. */
/* @@? 17:26 Error SyntaxError: Unexpected token ':'. */
/* @@? 17:26 Error SyntaxError: Unexpected token ':'. */
/* @@? 18:1 Error SyntaxError: Unexpected token '}'. */
/* @@? 20:50 Error TypeError: need to specify target type for class composite */
/* @@? 21:5 Error SyntaxError: Unexpected token. */
/* @@? 21:6 Error SyntaxError: Unexpected token, expected ':'. */
/* @@? 21:28 Error SyntaxError: Unexpected token. */
/* @@? 21:31 Error SyntaxError: Unexpected token ']'. */
/* @@? 21:31 Error SyntaxError: Unexpected token ']'. */
/* @@? 21:31 Error SyntaxError: Unexpected token ']'. */
/* @@? 21:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 21:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 21:43 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 23:6 Error SyntaxError: Unexpected token ','. */
/* @@? 23:6 Error SyntaxError: Unexpected token ','. */
/* @@? 24:21 Error TypeError: Bad operand type, the type of the operand must be numeric type. */
/* @@? 24:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:32 Error SyntaxError: Unexpected token ':'. */
/* @@? 25:1 Error SyntaxError: Unexpected token '}'. */
