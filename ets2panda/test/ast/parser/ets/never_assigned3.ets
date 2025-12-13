/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

const n_1: boolean = neverFunction();
const n_2: byte = neverFunction();
const n_3: short = neverFunction();
const n_4: int = neverFunction();
const n_5: float = neverFunction();
const n_6: double = neverFunction();
const n_7: long = neverFunction();
const n_8: string = neverFunction();

let m: never = neverFunction();

function test(a: Any) {
    
}

function neverFunction(): never {
  throw new Error("This function never returns");
}

test(n_1);
test(n_2);
test(n_3);
test(n_4);
test(n_5);
test(n_6);
test(n_7);
test(n_8);

test(m);