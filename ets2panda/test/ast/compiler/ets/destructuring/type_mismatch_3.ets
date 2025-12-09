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

function foo() : String {
    return "foo";
}

function bar() : number {
    return 42;
}

function main() {
  let a: number, b: number, c: number, d: number, str: string;
  [a, b, c] = foo();
  [d, str] = bar();
}

/* @@? 26:15 Error Syntax error ESY0049: Invalid destructuring assignment target. */
/* @@? 27:14 Error Syntax error ESY0049: Invalid destructuring assignment target. */
