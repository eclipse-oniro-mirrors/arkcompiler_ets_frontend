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

import {foo1} from "./foo"

function foo2() {}
overload foo {foo1, foo2} // compile-time error

namespace N {
export function fooN() {}
namespace M {
export function fooM() {}
}
overload goo {M.fooM, fooN} // compile-time error
}
overload bar {foo2, N.fooN} // compile-time error

/* @@? 19:15 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 26:15 Error Semantic error ESE503573: All overloaded functions must be declared in the same module or namespace scope. */
/* @@? 28:21 Error Semantic error ESE503573: All overloaded functions must be declared in the same module or namespace scope. */
