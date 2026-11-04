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

function doubleLeftBigint(): void {
    let a: boolean = /* @@ label1 */2.5 < 1n;
    let b: boolean = /* @@ label2 */2.5 > 1n;
    let c: boolean = /* @@ label3 */2.5 <= 1n;
    let d: boolean = /* @@ label4 */2.5 >= 1n;
}

/* @@@ label1 Error Semantic error ESE0105: Operator '<' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label2 Error Semantic error ESE0105: Operator '>' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label3 Error Semantic error ESE0105: Operator '<=' cannot be applied to types 'Double' and 'BigInt'. */
/* @@@ label4 Error Semantic error ESE0105: Operator '>=' cannot be applied to types 'Double' and 'BigInt'. */
