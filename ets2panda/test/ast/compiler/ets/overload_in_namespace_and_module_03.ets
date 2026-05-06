/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import {foo1} from "./foo"

function foo2() {}
overload foo {foo1, foo2} // compile-time error

namespace N {
export function fooN(): void{}
namespace M {
export function fooM(): void {}
}
overload goo {M.fooM, fooN} // compile-time error
}
overload bar {foo2, N.fooN} // compile-time error

/* @@? 19:15 Error Semantic error ESE0143: Unresolved reference foo1 */
/* @@? 19:15 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 26:15 Error Semantic error ESE503573: All overloaded functions must be declared in the same module or namespace scope. */
/* @@? 28:21 Error Semantic error ESE503573: All overloaded functions must be declared in the same module or namespace scope. */
/* @@? 26:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => undefined' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
/* @@? 28:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => undefined' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
