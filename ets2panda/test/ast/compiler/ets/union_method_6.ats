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

class A {
    foo(v: string) { console.log("A:" + v) }
}

class C {
    foo)v: string) { console.log("C:" + v) }
}

function test1(v: A|C) {
    v.foo("123")
}

/* @@? 21:8 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:8 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 21:18 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 21:20 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 21:29 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:29 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 21:33 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 21:34 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 21:34 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 21:34 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 21:34 Error Syntax error ESY0227: Unexpected token 'C:'. */
/* @@? 21:39 Error Syntax error ESY0227: Unexpected token '+'. */
/* @@? 21:41 Error Semantic error ESE0351: Variable 'v' has already been declared. */
/* @@? 21:42 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:42 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 22:1 Error Syntax error ESY0227: Unexpected token '}'. */
