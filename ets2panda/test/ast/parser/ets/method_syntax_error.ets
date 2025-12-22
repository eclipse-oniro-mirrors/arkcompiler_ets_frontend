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

class C {
    static class A<X extends A, Y extends B> {}
    static class B<X extends B> {}

    static foo1(U x):<U extends A<U, V>, V extends B<V>> Class<U> {
        return null;
    }

    static foo2(Class<U> c):<U extends A<U, V>, V extends B<V>> U {
        return null;
    }
}

/* @@? 17:12 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 17:30 Error Semantic error ESE0170: Type 'A<X,Y>' is generic but type argument were not provided. */
/* @@? 17:43 Error Semantic error ESE0170: Type 'B<X>' is generic but type argument were not provided. */
/* @@? 18:12 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 18:30 Error Semantic error ESE0170: Type 'B<X>' is generic but type argument were not provided. */
/* @@? 20:16 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 20:19 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 20:19 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 20:20 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:20 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 20:21 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 20:22 Error Syntax error ESY0227: Unexpected token '<'. */
/* @@? 20:24 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:25 Error Syntax error ESY0227: Unexpected token 'extends'. */
/* @@? 20:33 Error Semantic error ESE0369: Identifier 'A' has already been declared. */
/* @@? 20:34 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 20:40 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@? 20:44 Error Syntax error ESY0227: Unexpected token 'extends'. */
/* @@? 20:44 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 20:44 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 20:52 Error Semantic error ESE0369: Identifier 'B' has already been declared. */
/* @@? 20:53 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 20:55 Error Syntax error ESY0228: Unexpected token, expected '>'. */
/* @@? 20:58 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@? 20:63 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 20:63 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 20:63 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 20:63 Error Syntax error ESY0227: Unexpected token '<'. */
/* @@? 20:64 Error Semantic error ESE0351: Variable 'U' has already been declared. */
/* @@? 20:65 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:65 Error Syntax error ESY0227: Unexpected token '>'. */
/* @@? 20:67 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 21:9 Error Syntax error ESY0227: Unexpected token 'return'. */
/* @@? 21:16 Error Syntax error ESY0227: Unexpected token 'null'. */
/* @@? 24:5 Error Syntax error ESY0227: Unexpected token 'static'. */
/* @@? 24:12 Error Syntax error ESY0227: Unexpected token 'foo2'. */
/* @@? 24:12 Error Semantic error ESE0143: Unresolved reference foo2 */
/* @@? 24:23 Error Semantic error ESE0371: Cannot find type 'U'. */
/* @@? 24:26 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 24:26 Error Syntax error ESY0227: Unexpected token 'c'. */
/* @@? 24:26 Error Semantic error ESE0143: Unresolved reference c */
/* @@? 24:27 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 24:28 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 24:30 Error Semantic error ESE0143: Unresolved reference U */
/* @@? 24:32 Error Syntax error ESY0227: Unexpected token 'extends'. */
/* @@? 24:40 Error Syntax error ESY0227: Unexpected token 'A'. */
/* @@? 24:40 Error Semantic error ESE0143: Unresolved reference A */
/* @@? 24:42 Error Semantic error ESE0371: Cannot find type 'U'. */
/* @@? 24:45 Error Semantic error ESE0371: Cannot find type 'V'. */
/* @@? 24:47 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 24:49 Error Syntax error ESY0227: Unexpected token 'V'. */
/* @@? 24:49 Error Semantic error ESE0143: Unresolved reference V */
/* @@? 24:51 Error Syntax error ESY0227: Unexpected token 'extends'. */
/* @@? 24:59 Error Syntax error ESY0227: Unexpected token 'B'. */
/* @@? 24:59 Error Semantic error ESE0143: Unresolved reference B */
/* @@? 24:61 Error Semantic error ESE0371: Cannot find type 'V'. */
/* @@? 24:67 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 25:9 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 25:16 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 27:1 Error Syntax error ESY0227: Unexpected token '}'. */
