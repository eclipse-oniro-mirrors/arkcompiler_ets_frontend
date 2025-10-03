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

function foo(...^number: int[]): int {
    return number[0]
}


/* @@? 16:10 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 16:14 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 16:17 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 16:18 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 16:18 Error Syntax error ESY0067: Rest parameter must be the last formal parameter. */
/* @@? 16:31 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 16:32 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 16:34 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 16:38 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 17:5 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 17:12 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 17:12 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 17:12 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */