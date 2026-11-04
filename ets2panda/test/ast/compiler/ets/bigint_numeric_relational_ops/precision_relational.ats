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

function precisionLoss(): void {
    const precisionTest1: bigint = 9007199254740992n;
    const precisionTest2: number = 9007199254740992;
    let a: boolean = /* @@ label1 */precisionTest1 < precisionTest2;
    let b: boolean = /* @@ label2 */precisionTest1 <= precisionTest2;
    let c: boolean = /* @@ label3 */precisionTest1 > precisionTest2;
    let d: boolean = /* @@ label4 */precisionTest1 >= precisionTest2;
}

function unsafeRange(): void {
    const unsafeBig1: bigint = 9007199254740993n;
    const unsafeNum1: number = 9007199254740993;
    let a: boolean = /* @@ label5 */unsafeBig1 < unsafeNum1;
    let b: boolean = /* @@ label6 */unsafeBig1 > unsafeNum1;
    let c: boolean = /* @@ label7 */unsafeBig1 <= unsafeNum1;
    let d: boolean = /* @@ label8 */unsafeBig1 >= unsafeNum1;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
