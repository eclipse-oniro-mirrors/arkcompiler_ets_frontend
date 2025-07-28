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

/* @@? 16:24 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 16:42 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 16:45 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 18:33 Error TypeError: Cannot find type 'Observable'. */
/* @@? 18:33 Error TypeError: The super type of 'ObservableImpl' class is not extensible. */
/* @@? 20:24 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 20:27 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 20:29 Error SyntaxError: Unexpected token 'in'. */
/* @@? 20:41 Error SyntaxError: Unexpected token '&'. */
/* @@? 20:41 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 20:41 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 20:44 Error SyntaxError: Unexpected token ')'. */
/* @@? 20:44 Error SyntaxError: Field type annotation expected. */
/* @@? 20:45 Error SyntaxError: Unexpected token ']'. */
/* @@? 20:46 Error SyntaxError: Unexpected token ':'. */
/* @@? 20:48 Error SyntaxError: Call signatures in object types are not supported. Use '$_invoke' method instead. */
/* @@? 20:51 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 20:51 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 20:51 Error SyntaxError: Unexpected token '&'. */
/* @@? 20:54 Error SyntaxError: Unexpected token ')'. */
/* @@? 20:54 Error SyntaxError: Field type annotation expected. */
/* @@? 20:56 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 20:57 Error SyntaxError: Unexpected token ']'. */
/* @@? 24:36 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 24:42 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 24:42 Error SyntaxError: Unexpected token ']'. */
/* @@? 24:43 Error SyntaxError: Unexpected token ']'. */
/* @@? 24:45 Error SyntaxError: Unexpected token 'extends'. */
/* @@? 24:53 Error SyntaxError: Unexpected token 'object'. */
/* @@? 24:53 Error TypeError: Type name 'object' used in the wrong context */
/* @@? 24:62 Error TypeError: Unresolved reference keyof */
/* @@? 24:68 Error SyntaxError: Unexpected token. */
/* @@? 24:68 Error TypeError: Unresolved reference T */
/* @@? 24:68 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 24:70 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 24:73 Error SyntaxError: Unexpected token ':'. */
/* @@? 24:75 Error SyntaxError: Class cannot be used as object. */
/* @@? 24:75 Error SyntaxError: Unexpected token 'never'. */
