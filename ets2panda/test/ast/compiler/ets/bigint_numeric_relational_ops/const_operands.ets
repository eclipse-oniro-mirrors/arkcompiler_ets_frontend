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

function constOperands(): void {
    const b = 1n;
    const n = 2;
    let a: boolean = /* @@ label1 */b < n;
    let c: boolean = /* @@ label2 */b > n;
    let d: boolean = /* @@ label3 */b <= n;
    let e: boolean = /* @@ label4 */b >= n;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Int'. */
