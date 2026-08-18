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

function bigintLeftDouble(): void {
    let a: boolean = /* @@ label1 */1n < 2.5;
    let b: boolean = /* @@ label2 */1n > 2.5;
    let c: boolean = /* @@ label3 */1n <= 2.5;
    let d: boolean = /* @@ label4 */1n >= 2.5;
    let e: boolean = /* @@ label5 */1n <= 1.5;
    let f: boolean = /* @@ label6 */1n >= 0.5;
    let g: boolean = /* @@ label7 */1n <= 0.5;
    let h: boolean = /* @@ label8 */1n >= -0.5;
    let i: boolean = /* @@ label9 */1n < 1.2;
    let j: boolean = /* @@ label10 */1n > 1.2;
    let k: boolean = /* @@ label11 */2n < 1.9;
    let l: boolean = /* @@ label12 */2n > 1.9;
    let m: boolean = /* @@ label13 */0n < 0.1;
    let n: boolean = /* @@ label14 */0n > -0.1;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label9 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label10 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label11 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label12 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label13 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label14 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
