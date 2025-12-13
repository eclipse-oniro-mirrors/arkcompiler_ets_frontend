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
    public x : int;

    public constructor() {
        this(5);
    }

    public constructor(
        let tuple: {{c.to_type}} = {{caches.value}};
        let a : A = new A(...tuple);
        let method_result = A.bar(...tuple);
    ) {
        this.x = a;
    }
}

/* @@? 23:23 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 24:9 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 24:9 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 24:9 Error Syntax error ESY0227: Unexpected token 'let'. */
/* @@? 24:9 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 24:20 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 24:21 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 24:23 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 24:23 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 24:31 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 24:34 Error Syntax error ESY0227: Unexpected token '='. */
/* @@? 24:36 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 24:38 Error Semantic error ESE0143: Unresolved reference caches */
/* @@? 25:21 Error Semantic error ESE0127: No matching construct signature for class_cyclic_constructor.A(...tuple) */
/* @@? 25:30 Error Semantic error ESE0143: Unresolved reference tuple */
/* @@? 26:31 Error Semantic error ESE0087: Property 'bar' does not exist on type 'A' */
/* @@? 27:5 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 27:7 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 28:9 Error Semantic error ESE0202: 'this' cannot be referenced from a static context */
/* @@? 30:1 Error Syntax error ESY0227: Unexpected token '}'. */
