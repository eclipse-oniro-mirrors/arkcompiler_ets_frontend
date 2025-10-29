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


/* @@? 16:35 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@? 17:5 Error Syntax error ESY0016: Unexpected token. */
/* @@? 17:6 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 17:22 Error Syntax error ESY0016: Unexpected token. */
/* @@? 17:25 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 17:26 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 17:28 Error Syntax error ESY0227: Unexpected token '1'. */
/* @@? 18:1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 20:50 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@? 21:5 Error Syntax error ESY0016: Unexpected token. */
/* @@? 21:6 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 21:20 Error Semantic error ESE0347: Wrong operand type for unary expression */
/* @@? 21:28 Error Syntax error ESY0016: Unexpected token. */
/* @@? 21:31 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 21:32 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 21:34 Error Syntax error ESY0227: Unexpected token 'function'. */
/* @@? 21:43 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 23:6 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 23:6 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 24:20 Error Semantic error ESE0347: Wrong operand type for unary expression */
/* @@? 24:32 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 24:32 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 24:36 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 24:36 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 24:38 Error Syntax error ESY0227: Unexpected token 'string'. */
/* @@? 24:38 Error Semantic error ESE0144: Type name 'string' used in the wrong context */
/* @@? 24:44 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 24:45 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 24:46 Error Syntax error ESY0227: Unexpected token 'number'. */
/* @@? 24:46 Error Semantic error ESE0132: The type of parameter 'number' cannot be inferred */
/* @@? 24:56 Error Semantic error ESE0143: Unresolved reference y */
/* @@? 25:1 Error Syntax error ESY0227: Unexpected token '}'. */