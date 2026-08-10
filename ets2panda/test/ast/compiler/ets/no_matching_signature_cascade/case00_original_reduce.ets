/**
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

function take(x: number): number {
    return x;
}

function main(): void {
    take(take([1.1, 2.2, 3.3].reduce((a: number, b: number): string => a + b, 0)));
}

/* @@? 21:15 Error Semantic error ESE0127: No matching call signature for reduce(((p1: Double, p2: Double) => String), Int) */
/* @@? 21:38 Error Semantic error ESE0046: Type '((p1: Double, p2: Double) => String)' is not compatible with type '((p1: Int, p2: Double, p3: Int, p4: ReadonlyArray<Double>) => Int)' at index 1 */
/* @@? 21:72 Error Semantic error ESE0091: Type 'Double' is not compatible with the enclosing method's return type 'String' */
