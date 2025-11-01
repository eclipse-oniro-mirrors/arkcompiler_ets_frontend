/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function gf(x: String, fn: (x: String = /* @@ label */"TEST") => Int): Int {
  return fn(x)
}

function main(): void {
   gf("abc" as String, (x: String): Int => { return x=="TEST"?0:1})
}

/* @@@ label Error SyntaxError: Default value is allowed only for optional parameters. */
/* @@? 21:24 Error TypeError: Type '((p1: String) => Int)' is not compatible with type '((p1: String|undefined) => Int)' at index 2 */
