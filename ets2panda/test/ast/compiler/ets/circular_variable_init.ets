/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class A {
    public a = this.b
    public b = this.a
}

class B {
    public static a = B.b
    public static b = B.a
}

class C {
    public static a = C.b
    public static b = 0
}

let globala = globalb

let globalc = globald
let globald = globalc

const consta = constb;
const constb = consta;

function main() {
    let maina = mainb;

    let mainc = maind;
    let maind = mainc;
}

/* @@? 17:16 Warning Warning: The instance field initializer expression cannot use the this. */
/* @@? 18:12 Error TypeError: Circular dependency detected for identifier: b */
/* @@? 18:16 Warning Warning: The instance field initializer expression cannot use the this. */
/* @@? 23:19 Error TypeError: Circular dependency detected for identifier: b */
/* @@? 31:15 Error TypeError: Unresolved reference globalb */
/* @@? 34:5 Error TypeError: Circular dependency detected for identifier: globald */
/* @@? 37:7 Error TypeError: Circular dependency detected for identifier: constb */
/* @@? 40:17 Error TypeError: Unresolved reference mainb */
/* @@? 42:17 Error TypeError: Variable 'maind' is accessed before it's initialization. */
/* @@? 43:9 Error TypeError: Circular dependency detected for identifier: maind */
