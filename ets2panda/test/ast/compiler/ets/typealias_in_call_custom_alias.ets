/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// A user-declared type alias used as a value inside a template literal argument of a
// multi-overload user function (not the builtin console.log).

type MyAlias = int

function f(a: int) {}
function f(s: String) {}

function main() {
    f(`${(typeof 1) === MyAlias ? 'a' : 'b'}`)
}

/* @@? 25:25 Error Semantic error ESE0144: Type name 'MyAlias' used in the wrong context */
