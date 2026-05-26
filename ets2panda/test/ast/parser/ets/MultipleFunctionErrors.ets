/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

function /* @@ label */(): void {
}

function /* @@ label1 */(): /* @@ label2 */{
    /* @@ label3 */return /* @@ label4 */77;
}


/* @@@ label Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@@ label1 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@@ label1 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@@ label1 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => *ERROR_TYPE*' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
/* @@@ label2 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@@ label3 Error Syntax error ESY0227: Unexpected token 'return'. */
/* @@@ label4 Error Syntax error ESY0227: Unexpected token '77'. */
