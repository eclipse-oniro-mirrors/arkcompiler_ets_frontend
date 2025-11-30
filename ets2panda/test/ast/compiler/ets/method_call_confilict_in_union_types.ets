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
    foo under the licenses { }
    foo(v: int): void {}
}

class B {
    foo(v: string): void { }
}

function foo(x: A | B) {
    x.foo("123")
}


/* @@? 17:8 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:14 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:18 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:27 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:28 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 18:9 Error Semantic error ESE0143: Unresolved reference v */
/* @@? 18:10 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 18:10 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 18:12 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 18:15 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 18:16 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 18:18 Error Syntax error ESY0227: Unexpected token 'void'. */
/* @@? 18:23 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 19:1 Error Syntax error ESY0227: Unexpected token '}'. */
