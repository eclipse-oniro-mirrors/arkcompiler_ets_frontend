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

function foo(a: int[] | string): void {}

function bar(a: int[] | string[]): void {}

foo([1, undefined])
bar([1, undefined])
/* @@? 20:9 Error Semantic error ESE0227: Array element at index 1 with type 'undefined' is not compatible with the target array element type 'Int' */
/* @@? 21:1 Error Semantic error ESE0127: No matching call signature for bar(Array<Int|undefined>) */
/* @@? 21:5 Error Semantic error ESE0046: Type 'Array<Int|undefined>' is not compatible with type 'Array<Int>|Array<String>' at index 1 */
