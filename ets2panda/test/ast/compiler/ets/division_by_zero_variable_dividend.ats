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

// Integer division (and modulo) by a compile-time constant zero must be a
// compile-time error (see spec 7_expressions.rst, "Division"), even when the
// dividend is a runtime value such as a local variable. This covers literal
// `0` and `const` zero divisors, all signed integer types, and the compound
// assignment operators `/=` and `%=`.

function main() : void {
    let x: int = 10;
    const zero: int = 0;
    let div_lit_zero : int = /* @@ label1 */x / 0;
    let mod_lit_zero : int = /* @@ label2 */x % 0;
    let div_const_zero: int = /* @@ label3 */x / zero;
    let mod_const_zero: int = /* @@ label4 */x % zero;
    let b: byte = 1;
    let s: short = 1;
    let l: long = 1;
    let div_byte_zero : int = /* @@ label5 */b / 0;
    let mod_short_zero: int = /* @@ label6 */s % zero;
    let div_long_zero : long = /* @@ label7 */l / 0;
    /* @@ label8 */x /= 0;
    /* @@ label9 */x %= zero;
}


/* @@@ label1 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label2 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label3 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label4 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label5 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label6 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label7 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label8 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
/* @@@ label9 Error Syntax error ESY0273: Division by zero is not allowed in integer arithmetic. */
