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

// Recursive generic union type test case migrated for ArkTS 1.2

// ArkTS does not support recursive type aliases or type declarations inside namespaces.
// We flatten the structure and define types globally to avoid unsupported features.

type Container<T> = T | { [i: string]: Container<T> };

declare namespace Test1 {
}

/* @@? 21:25 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 21:28 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 21:49 Error SyntaxError: Unexpected token '<'. */
/* @@? 21:51 Error SyntaxError: Field type annotation expected. */
/* @@? 21:51 Error SyntaxError: Unexpected token '>'. */
