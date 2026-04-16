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

declare function testRest<T>(a: (t: T, t1: T, ...ts: T[]) => void): T

testRest((t1:number, t2:boolean, t3) => {})

/* @@? 18:1 Error Semantic error ESE0127: No matching call signature for testRest(((p1: Double, p2: Boolean, p3: *ERROR_TYPE*) => undefined)) */
/* @@? 18:1 Error Semantic error ESE8745: Type parameter 'T' can not be inferred for the call to 'testRest'. Specify the type argument explicitly. */
/* @@? 18:10 Error Semantic error ESE0046: Type '((p1: Double, p2: Boolean, p3: T) => undefined)' is not compatible with type '((p1: T, p2: T, ...p3: Array<T>) => undefined)' at index 1 */
/* @@? 18:34 Error Semantic error ESE0132: The type of parameter 't3' cannot be inferred */
