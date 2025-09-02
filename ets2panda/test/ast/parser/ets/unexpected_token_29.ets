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

function main(): void {
    for (let i: int = 0 /* @@ label1 */i < 10 ^ i++/* @@ label2 */) {
        console/*@@ label3 */.log("aaaaaaaa")
    }
/*@@ label4 */}

/* @@@ label1 Error SyntaxError: Expected ';', got 'identification literal'. */
/* @@? 17:44 Error TypeError: Bad operand type, the types of the operands must be numeric type. */
/* @@@ label2 Error SyntaxError: Unexpected token, expected ';'. */
/* @@? 17:69 Error TypeError: need to specify target type for class composite */
/* @@? 18:30 Error SyntaxError: Unexpected token, expected ':'. */
/* @@? 20:15 Error SyntaxError: Expected ')', got '}'. */
/* @@? 20:15 Error SyntaxError: Unexpected token '}'. */
/* @@? 30:1 Error SyntaxError: Expected '}', got 'end of stream'. */
