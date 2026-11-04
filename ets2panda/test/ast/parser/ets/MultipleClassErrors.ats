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

class A {
    /* @@ label */function f(): int {
        return 55
    }

    /* @@ label1 */let x: number = 7
    /* @@ label2 */let y = "abc"

    a: number = "abc"
    b: int = 44
}

class /* @@ label3 */77 {}

class /* @@ label4 */{/* @@ label5 */}


/* @@@ label Error Syntax error ESY0227: Unexpected token 'function'. */
/* @@@ label1 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@@ label2 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 24:17 Error Semantic error ESE0318: Type '"abc"' cannot be assigned to type 'Double' */
/* @@@ label3 Error Syntax error ESY0315: Number, string or computed value property name '77' is not allowed, use classes to access data by property names that are identifiers */
/* @@@ label3 Error Syntax error ESY0224: Identifier expected, got 'number literal'. */
/* @@@ label4 Error Syntax error ESY0224: Identifier expected, got '{'. */
/* @@@ label5 Error Syntax error ESY0230: Expected '{', got '}'. */
/* @@? 41:79 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */