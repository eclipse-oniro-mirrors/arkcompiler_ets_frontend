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

function hugeBigints(): void {
    const MAX_VALUE: number = Number.MAX_VALUE;
    const largeBigInt: bigint =
        10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000n;
    const veryLargeBigInt: bigint = 1234567890123456789012345678901234567890n;
    const verySmallBigInt: bigint = -1234567890123456789012345678901234567890n;
    let a: boolean = /* @@ label1 */largeBigInt < MAX_VALUE;
    let b: boolean = /* @@ label2 */veryLargeBigInt < Infinity;
    let c: boolean = /* @@ label3 */veryLargeBigInt > Number.MAX_SAFE_INTEGER;
    let d: boolean = /* @@ label4 */verySmallBigInt > -Infinity;
    let e: boolean = /* @@ label5 */verySmallBigInt < Number.MIN_SAFE_INTEGER;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
