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
        console/* @@ label3 */.log/* @@ label4 */(/* @@ label5 */"aaaaaaaa"/* @@ label6 */)
    }
/* @@ label7 */}


/* @@@ label1 Error Syntax error ESY0230: Expected ';', got 'identification literal'. */
/* @@? 17:44 Error Semantic error ESE0107: Bad operand type, the types of the operands must be numeric type. */
/* @@@ label2 Error Syntax error ESY0228: Unexpected token, expected ';'. */
/* @@? 17:69 Error Semantic error ESE0062: need to specify target type for class composite */
/* @@@ label3 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@@ label4 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@@ label4 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@@ label4 Error Syntax error ESY0230: Expected '{', got '('. */
/* @@@ label4 Error Syntax error ESY0016: Unexpected token. */
/* @@@ label5 Error Syntax error ESY0230: Expected ')', got 'string literal'. */
/* @@@ label6 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@@ label7 Error Syntax error ESY0227: Unexpected token '}'. */
