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

function typedVariables(): void {
    let b: bigint = 1n;
    let n: number = 2;
    let a: boolean = /* @@ label1 */b < n;
    let c: boolean = /* @@ label2 */b > n;
    let d: boolean = /* @@ label3 */b <= n;
    let e: boolean = /* @@ label4 */b >= n;
    let f: boolean = /* @@ label5 */n < b;
    let g: boolean = /* @@ label6 */n > b;
    let h: boolean = /* @@ label7 */n <= b;
    let i: boolean = /* @@ label8 */n >= b;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'Double' and 'BigInt'. */
