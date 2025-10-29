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

let r1: Record<string, string> = { 'one': '1' }
console.log(...r1)                                 // crash - Issue #26773
let r2: Record<string, string> = { 'two': '2' }
let r3: Record<string, string> = { ...r1, ...r2 }
console.log(...r3)                                 // crash - Issue #26773



/* @@? 17:1 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@? 17:1 Error Semantic error ESE0127: No matching call signature for log(...r1) */
/* @@? 17:1 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@? 17:13 Error Semantic error ESE0323: Type 'Record<String,String>' is not compatible with rest parameter type 'Array<Any>' at index 1 */
/* @@? 17:13 Error Semantic error ESE0049: Spread expression can be applied only to array or tuple type, but 'Record<String,String>' is provided */
/* @@? 20:1 Error Semantic error ESE0127: No matching call signature for log(...r3) */
/* @@? 20:1 Error Semantic error ESE44192: The function or method being called needs a rest parameter to accept arguments passed via the spread operator. */
/* @@? 20:13 Error Semantic error ESE0323: Type 'Record<String,String>' is not compatible with rest parameter type 'Array<Any>' at index 1 */
/* @@? 20:13 Error Semantic error ESE0049: Spread expression can be applied only to array or tuple type, but 'Record<String,String>' is provided */