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

let prim_b: byte = 8;
let non_prim_b: Byte = -8;

let prim_s: short = 42;
let non_prim_s: Short = -42;


let prim_i: int = 65535;
let non_prim_i: Int = -65535;

let prim_l: long = 10000000;
let non_prim_l: Long = -10000000;



// see 3.1.2 Float types

let prim_f: float = 0.0;
let non_prim_f: Float = -0.0;

let prim_d: double = 0.0;
let non_prim_d: Double = -0.0;



// see 3.1.3 Boolean type

let /* @@ label */prim_b: bool = true;
let non_prim_b: Bool = false;



// see 3.1.4 Char type
let prim_c: char = c'a';
let non_prim_c: Char = c'b';



// see 3.1.5 Void type
function v(): void {}

/* @@? 35:21 Error TypeError: Type 'Double' cannot be assigned to type 'Float' */
/* @@? 36:25 Error TypeError: Type 'Double' cannot be assigned to type 'Float' */
/* @@@ label Error TypeError: Variable 'prim_b' has already been declared. */
/* @@? 45:27 Error TypeError: Cannot find type 'bool'. */
/* @@? 45:34 Error TypeError: Type 'Boolean' cannot be assigned to type 'Byte' */
/* @@? 46:5 Error TypeError: Variable 'non_prim_b' has already been declared. */
/* @@? 46:17 Error TypeError: Cannot find type 'Bool'. */
/* @@? 46:24 Error TypeError: Type 'Boolean' cannot be assigned to type 'Byte' */
