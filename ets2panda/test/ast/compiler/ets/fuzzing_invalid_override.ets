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

interface ReadonlyHashMap<K, V> extends Iterable<[K, V]> {
    forEach(callbackFn: (value: V, key: K, map: ReadonlyHashMap<K, V>) => void): void;
}

export default class HashMap<K, V> implements ReadonlyHashMap<K, V> {
    forEach(this.type === XmlDynamicType.START_AND_ATTRIBUTES): void {
        this.buckets.forEach((value: V, key: K) => {
            callbackFn(value, key, this);
        });
    }
}


/* @@? 20:69 Error Semantic error ESE0190: HashMap is not abstract and does not override abstract method forEach(callbackFn: ((p1: V, p2: K, p3: ReadonlyHashMap<K,V>) => void)): void in ReadonlyHashMap */
/* @@? 21:12 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 21:17 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 21:17 Error Syntax error ESY0099: The function parameter 'this' must explicitly specify the typeAnnotation. */
/* @@? 21:17 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 21:22 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:23 Error Syntax error ESY0227: Unexpected token '==='. */
/* @@? 21:41 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:41 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 21:62 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 21:62 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 21:63 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 21:65 Error Syntax error ESY0295: void is a predefined type, cannot be used as an identifier */
/* @@? 21:70 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 22:9 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 22:13 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 22:21 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 22:21 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 22:30 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 22:30 Error Syntax error ESY103145: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 22:30 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 22:30 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 22:49 Error Syntax error ESY0227: Unexpected token '=>'. */
/* @@? 22:52 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 23:23 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 23:29 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 23:34 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 23:40 Error Syntax error ESY0099: The function parameter 'this' must explicitly specify the typeAnnotation. */
/* @@? 24:10 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 25:5 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 26:1 Error Syntax error ESY0227: Unexpected token '}'. */
