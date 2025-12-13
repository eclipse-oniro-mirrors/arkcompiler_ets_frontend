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

function gen<T>(x: T): T { return x; }
function apply(fn: (n: number) => number): number { return fn(5); }

function main() {
  let bad = apply(gen<string>);
}

/* @@? 20:13 Error Semantic error ESE0127: No matching call signature for apply(((p1: String) => String)) */
/* @@? 20:19 Error Semantic error ESE0046: Type '((p1: String) => String)' is not compatible with type '((p1: Double) => Double)' at index 1 */
