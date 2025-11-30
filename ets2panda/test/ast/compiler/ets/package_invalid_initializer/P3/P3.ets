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

package P3

class A {
    data:number = 1;
}

function foo(): int {
    return 1;
}

export let nn: number = new A().data
export let nn1: int = foo();
export let nn2: number = 1;
export const c_nn;
export const c_nn1: number = foo();
export const c_nn2: number = 1; // initialized twice.
nn = foo();
foo();
console.log(foo())
static {
    c_nn2 = 2
}

export const var_tobe_shadowed: number;
static {
    let var_tobe_shadowed: number = 10;
    var_tobe_shadowed = 2;
}


/* @@? 29:14 Error Syntax error ESY0294: Missing initialization for const package property */
/* @@? 29:18 Error Syntax error ESY0105: Variable must be initialized or it's type must be declared. */
/* @@? 30:30 Error Syntax error ESY0288: Non-constant initializer of Package should be apply in Initializer Block. */
/* @@? 31:14 Error Semantic error ESE0297: Cannot reassign constant c_nn2 */
/* @@? 33:1 Error Syntax error ESY0291: Invalid package toplevel statement */
/* @@? 34:1 Error Syntax error ESY0291: Invalid package toplevel statement */
/* @@? 39:14 Error Syntax error ESY0294: Missing initialization for const package property */
