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

function f0<T>(p: T): T {
    return p;
}

function f1<T extends ((x: int) => int)>(p: T): T {
    return p;
}

function f2<T extends ((x: int) => int)[]>(p: T): T {
    return p;
}

function main() {
    let s0 = f0([x]);
    let s1 = f1([x]);
    let s2 = f2([x]);
    let v0 = f0([x => x]);
    let v1 = f1([x => x]);
    let v2 = f2([x => x]);
    let a = 1;
    let right1 = f0([a]);
    let right2 = f0([() => a]);
}

/* @@? 29:14 Error Semantic error ESE0127: No matching call signature for f0(...) */
/* @@? 29:18 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 30:14 Error Semantic error ESE0127: No matching call signature for f1(...) */
/* @@? 30:18 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 31:14 Error Semantic error ESE0127: No matching call signature for f2(...) */
/* @@? 31:18 Error Semantic error ESE0143: Unresolved reference x */
/* @@? 32:14 Error Semantic error ESE0127: No matching call signature for f0(Array<((p1: *ERROR_TYPE*) => void)>) */
/* @@? 32:17 Error Semantic error ESE0132: The type of parameter 'p' cannot be inferred */
/* @@? 33:14 Error Semantic error ESE0127: No matching call signature for f1(Array<((p1: *ERROR_TYPE*) => void)>) */
/* @@? 33:17 Error Semantic error ESE0132: The type of parameter 'p' cannot be inferred */
/* @@? 34:14 Error Semantic error ESE0127: No matching call signature for f2(Array<((p1: *ERROR_TYPE*) => void)>) */
/* @@? 34:17 Error Semantic error ESE0132: The type of parameter 'p' cannot be inferred */