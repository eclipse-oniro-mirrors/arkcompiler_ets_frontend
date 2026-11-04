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

class Base {
    foo1(a: number) { }
    foo2(a: string) { }
    overload foo(foo1, foo2)
}


/* @@? 19:17 Error Syntax error ESY0228: Unexpected token, expected '{'. */
/* @@? 19:17 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 19:17 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 19:18 Error Syntax error ESY0229: Unexpected token, expected ',' or '}'. */
/* @@? 19:18 Error Semantic error ESE0351: Variable 'foo1' has already been declared. */
/* @@? 19:22 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 19:22 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 19:24 Error Semantic error ESE0351: Variable 'foo2' has already been declared. */
/* @@? 19:28 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 19:28 Error Syntax error ESY0227: Unexpected token ')'. */