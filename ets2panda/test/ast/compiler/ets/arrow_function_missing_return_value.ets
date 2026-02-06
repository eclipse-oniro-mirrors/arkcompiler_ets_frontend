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
type A = number;
type B = number|undefined;
type C = Promise<A>;
type D = Promise<A>|Promise<void>;
const J = /* @@ label1 */():number => {}
let K = /* @@ label2 */():number => {}
function main(): number|undefined {
    let a = /* @@ label3 */():A => {}
    let b = ():B => {}
    let c = /* @@ label4 */():C => {}
    let d = ():D => {}
    let e = ():number|undefined => {}
    let f = ():void => {}
    let g = /* @@ label5 */():Promise<number> => {}
    let h = ():Promise<number>|Promise<void> => {}
    let i = ():Promise<void> => {}
    let j = /* @@ label6 */():number => {}
    const k = /* @@ label7 */():number => {}
}

/* @@@ label1 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label2 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label3 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label4 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label5 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label6 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */
/* @@@ label7 Error Semantic error ESE57199: Lambda with a non-void return type must return a value. */