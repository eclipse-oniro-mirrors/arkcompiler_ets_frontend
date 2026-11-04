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

function maxValue(): void {
    const MAX_VALUE: number = Number.MAX_VALUE;
    let a: boolean = /* @@ label1 */1n < MAX_VALUE;
    let b: boolean = /* @@ label2 */0n < MAX_VALUE;
    let c: boolean = /* @@ label3 */-1n < MAX_VALUE;
}

function minValue(): void {
    const MIN_VALUE: number = Number.MIN_VALUE;
    let a: boolean = /* @@ label4 */0n < MIN_VALUE;
    let b: boolean = /* @@ label5 */0n > MIN_VALUE;
    let c: boolean = /* @@ label6 */1n < MIN_VALUE;
    let d: boolean = /* @@ label7 */1n > MIN_VALUE;
    let e: boolean = /* @@ label8 */-1n < MIN_VALUE;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
