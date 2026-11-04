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

function maxSafe(): void {
    const MAX_SAFE: number = Number.MAX_SAFE_INTEGER;
    let a: boolean = /* @@ label1 */BigInt(MAX_SAFE) < MAX_SAFE;
    let b: boolean = /* @@ label2 */BigInt(MAX_SAFE) <= MAX_SAFE;
    let c: boolean = /* @@ label3 */BigInt(MAX_SAFE) > MAX_SAFE;
    let d: boolean = /* @@ label4 */BigInt(MAX_SAFE) >= MAX_SAFE;
    let e: boolean = /* @@ label5 */BigInt(MAX_SAFE - 1) < MAX_SAFE;
    let f: boolean = /* @@ label6 */BigInt(MAX_SAFE + 1) > MAX_SAFE;
}

function minSafe(): void {
    const MIN_SAFE: number = Number.MIN_SAFE_INTEGER;
    let a: boolean = /* @@ label7 */BigInt(MIN_SAFE) < MIN_SAFE;
    let b: boolean = /* @@ label8 */BigInt(MIN_SAFE) <= MIN_SAFE;
    let c: boolean = /* @@ label9 */BigInt(MIN_SAFE) > MIN_SAFE;
    let d: boolean = /* @@ label10 */BigInt(MIN_SAFE) >= MIN_SAFE;
    let e: boolean = /* @@ label11 */BigInt(MIN_SAFE - 1) < MIN_SAFE;
    let f: boolean = /* @@ label12 */BigInt(MIN_SAFE + 1) > MIN_SAFE;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label9 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label10 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label11 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label12 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
