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

// see 3.1.1 Integer types
let non_prim_b: Byte = -8;
let non_prim_s: Short = -42;
let non_prim_i: Int = -65535;
let non_prim_l: Long = -10000000;

// see 3.1.2 Float types
let non_prim_f: Float = -0.0;
let non_prim_d: Double = -0.0;

// see 3.1.3 Boolean type
let /* @@ label */non_prim_b: Bool = false;

// see 3.1.4 Char type
let non_prim_c: Char = c'b';

// see 3.2.1 String type
let s: String = "abc";

// see 3.2.2 Array types
let a: int[] = new int[5];


/* @@? 23:25 Error Semantic error ESE0318: Type 'Double' cannot be assigned to type 'Float' */
/* @@@ label Error Semantic error ESE0351: Variable 'non_prim_b' has already been declared. */
/* @@? 27:31 Error Semantic error ESE0371: Cannot find type 'Bool'. */
/* @@? 27:38 Error Semantic error ESE0318: Type 'Boolean' cannot be assigned to type 'Byte' */