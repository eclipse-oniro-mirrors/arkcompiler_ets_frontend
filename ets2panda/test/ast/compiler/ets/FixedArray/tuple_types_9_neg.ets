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


function main(): void {
    let a: [number, string, FixedArray<...Int>] = [2, "a", 3];
}

/* @@? 18:29 Error TypeError: FixedArray<T> must have only one type parameter. */
/* @@? 18:40 Error SyntaxError: Invalid Type. */
/* @@? 18:40 Error SyntaxError: Unexpected token, expected '>'. */
/* @@? 18:40 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 18:40 Error SyntaxError: Unexpected token '>'. */
/* @@? 18:43 Error SyntaxError: Unexpected token 'Int'. */
/* @@? 18:43 Error SyntaxError: Class cannot be used as object. */
/* @@? 18:43 Error TypeError: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */
/* @@? 18:47 Error SyntaxError: Unexpected token ']'. */
