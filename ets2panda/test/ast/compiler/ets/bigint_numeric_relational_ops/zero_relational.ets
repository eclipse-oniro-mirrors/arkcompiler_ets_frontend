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

function zero(): void {
    let a: boolean = /* @@ label1 */0n < 0;
    let b: boolean = /* @@ label2 */0n > 0;
    let c: boolean = /* @@ label3 */0n <= 0;
    let d: boolean = /* @@ label4 */0n >= 0;
    let e: boolean = /* @@ label5 */0n < -0;
    let f: boolean = /* @@ label6 */0n > -0;
    let g: boolean = /* @@ label7 */0n <= -0;
    let h: boolean = /* @@ label8 */0n >= -0;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label7 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label8 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Int'. */
