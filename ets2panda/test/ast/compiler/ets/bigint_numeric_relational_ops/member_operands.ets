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

class Holder {
    b: bigint = 1n;
    n: number = 2;
}

function byField(): void {
    let h: Holder = new Holder();
    let a: boolean = /* @@ label1 */h.b < h.n;
    let c: boolean = /* @@ label2 */h.b > h.n;
    let d: boolean = /* @@ label3 */h.b <= h.n;
    let e: boolean = /* @@ label4 */h.b >= h.n;
}

function byIndex(): void {
    let arr: bigint[] = [1n, 2n];
    let num: number = 3;
    let a: boolean = /* @@ label5 */arr[0] < num;
    let c: boolean = /* @@ label6 */arr[0] > num;
    let d: boolean = /* @@ label7 */arr[0] <= num;
    let e: boolean = /* @@ label8 */arr[0] >= num;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Double'. */
