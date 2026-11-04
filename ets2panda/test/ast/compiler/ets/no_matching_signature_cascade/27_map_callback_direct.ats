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

function main(): void {
  [1.1, 2.2].map((x: number): string => x + 1);
}

/* @@? 17:3 Error Semantic error ESE0127: No matching call signature for map(((p1: Double) => String)) */
/* @@? 17:3 Error Semantic error ESE8745: Type parameter 'U' can not be inferred for the call to 'map'. Specify the type argument explicitly. */
/* @@? 17:18 Error Semantic error ESE0046: Type '((p1: Double) => String)' is not compatible with type '((p1: Double, p2: Int, p3: ReadonlyArray<Double>) => U)' at index 1 */
/* @@? 17:41 Error Semantic error ESE0091: Type 'Double' is not compatible with the enclosing method's return type 'String' */
