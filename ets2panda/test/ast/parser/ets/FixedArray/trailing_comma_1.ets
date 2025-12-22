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

function foo(...number: FixedArray<int>,): int {
    return number[0]
}

function foo2(number: int,): int {
    return number
}

function foo3(, number: int): int {
    return number
}

let a = [1,, 2,]
foo(...a,)
foo(a[0],)
foo(a[0], a[1], ...a,)
foo(a[0],,a[0])
foo(a[0],,)
foo(,a[0])
foo(a[0] a[1])
foo(,)


/* @@? 16:40 Error Syntax error ESY0067: Rest parameter must be the last formal parameter. */
/* @@? 24:15 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 24:15 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 28:9 Error Syntax error ESY167720: Omitted elements are not supported in arrays. */
/* @@? 29:1 Error Semantic error ESE0127: No matching call signature for foo(Double) */
/* @@? 29:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 30:1 Error Semantic error ESE0127: No matching call signature for foo(Double) */
/* @@? 30:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 31:1 Error Semantic error ESE0127: No matching call signature for foo(Double, Double, Double) */
/* @@? 31:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 32:1 Error Semantic error ESE0127: No matching call signature for foo(Double, ...) */
/* @@? 32:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 32:10 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 32:11 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 32:11 Error Syntax error ESY0227: Unexpected token 'a'. */
/* @@? 32:15 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 33:1 Error Semantic error ESE0127: No matching call signature for foo(Double, ...) */
/* @@? 33:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 33:10 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 34:5 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 34:6 Error Syntax error ESY0227: Unexpected token 'a'. */
/* @@? 34:6 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 34:10 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 35:1 Error Semantic error ESE0127: No matching call signature for foo(Double) */
/* @@? 35:5 Error Semantic error ESE0323: Type 'Double' is not compatible with rest parameter type 'Int' at index 1 */
/* @@? 35:10 Error Syntax error ESY0227: Unexpected token 'a'. */
/* @@? 35:10 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 35:14 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 36:5 Error Syntax error ESY0227: Unexpected token ','. */
