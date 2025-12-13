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

function foo<T>(t: T | {}, k: keyof (T | {})) {}

class ObservableImpl<T> extends Observable<T> {
  extendObservable<U extends object>(extraProps: U) {
    new ObservableImpl<{ [K in keyof (T & U)]: (T & U)[K] }>();
 }
}

type NestedKey<T, K extends keyof T> = T[K] extends object ? keyof T[K] : never;

/* @@? 16:24 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 16:42 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 16:45 Error Semantic error ESE0276: The `keyof` keyword can only be used for class or interface type. */
/* @@? 18:33 Error Semantic error ESE0371: Cannot find type 'Observable'. */
/* @@? 18:33 Error Semantic error ESE0176: The super type of 'ObservableImpl' class is not extensible. */
/* @@? 20:24 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 20:27 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 20:29 Error Syntax error ESY0227: Unexpected token 'in'. */
/* @@? 20:41 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 20:41 Error Syntax error ESY0227: Unexpected token '&'. */
/* @@? 20:41 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 20:44 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 20:44 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:45 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 20:46 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 20:48 Error Syntax error ESY103145: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 20:51 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 20:51 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 20:51 Error Syntax error ESY0227: Unexpected token '&'. */
/* @@? 20:54 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 20:54 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:56 Error Semantic error ESE0343: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 20:57 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 24:36 Error Semantic error ESE0276: The `keyof` keyword can only be used for class or interface type. */
/* @@? 24:42 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 24:42 Error Syntax error ESY0228: Unexpected token, expected ']'. */
/* @@? 24:43 Error Syntax error ESY0227: Unexpected token ']'. */
/* @@? 24:45 Error Syntax error ESY0227: Unexpected token 'extends'. */
/* @@? 24:53 Error Syntax error ESY0227: Unexpected token 'object'. */
/* @@? 24:53 Error Semantic error ESE0144: Type name 'object' used in the wrong context */
/* @@? 24:62 Error Semantic error ESE0143: Unresolved reference keyof */
/* @@? 24:68 Error Syntax error ESY0016: Unexpected token. */
/* @@? 24:68 Error Semantic error ESE0143: Unresolved reference T */
/* @@? 24:68 Error Semantic error ESE0252: Indexed access is not supported for such expression type. */
/* @@? 24:73 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 24:75 Error Syntax error ESY0227: Unexpected token 'never'. */
/* @@? 24:75 Error Syntax error ESY0318: Class cannot be used as object. */
