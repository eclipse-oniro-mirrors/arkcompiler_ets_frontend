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

function byParam(b: bigint, n: number): void {
    let a: boolean = /* @@ label1 */b < n;
    let c: boolean = /* @@ label2 */b > n;
    let d: boolean = /* @@ label3 */b <= n;
    let e: boolean = /* @@ label4 */b >= n;
}

function byParamReversed(b: bigint, n: number): void {
    let a: boolean = /* @@ label5 */n < b;
    let c: boolean = /* @@ label6 */n > b;
    let d: boolean = /* @@ label7 */n <= b;
    let e: boolean = /* @@ label8 */n >= b;
}

function getBig(): bigint {
    return 1n;
}

function getNum(): number {
    return 2;
}

function byCall(): void {
    let a: boolean = /* @@ label9 */getBig() < getNum();
    let c: boolean = /* @@ label10 */getBig() > getNum();
    let d: boolean = /* @@ label11 */getBig() <= getNum();
    let e: boolean = /* @@ label12 */getBig() >= getNum();
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label9 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label10 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label11 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label12 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
