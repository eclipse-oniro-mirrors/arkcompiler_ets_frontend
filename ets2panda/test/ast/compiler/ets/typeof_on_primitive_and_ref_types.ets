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

class A {}

function main() {
    let a1: A ;
    let a2: A ;
    a1 = new A();

    let b1 : int;
    let b2 : int;
    b1 = 1;

    let c1 = a1;
    let c2 = /* @@ label1 */a2;

    typeof a1;
    typeof /* @@ label2 */a2;
    typeof b1;
    typeof b2;
    typeof c1;
    typeof c2;
}



/* @@@ label1 Error Semantic error ESE0375: Variable 'a2' is used before being assigned. */
/* @@@ label2 Error Semantic error ESE0375: Variable 'a2' is used before being assigned. */