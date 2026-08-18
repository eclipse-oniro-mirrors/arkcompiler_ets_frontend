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

function negative(): void {
    let a: boolean = /* @@ label1 */-1n < -1;
    let b: boolean = /* @@ label2 */-1n <= -1;
    let c: boolean = /* @@ label3 */-1n > -1;
    let d: boolean = /* @@ label4 */-1n >= -1;
    let e: boolean = /* @@ label5 */-1n < -0.5;
    let f: boolean = /* @@ label6 */-1n > -1.5;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'BigInt' and 'Int'. */
/* @@@ label5 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'BigInt' and 'Double'. */
/* @@@ label6 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'BigInt' and 'Double'. */
