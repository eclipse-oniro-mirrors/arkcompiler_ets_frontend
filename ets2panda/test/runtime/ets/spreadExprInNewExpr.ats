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
    fld: number[];
    constructor(...p: number[]) {
        this.fld = p;
    }
}

function foo(): number[] {
    return [1.0, 2.0, 3.0];
}
let nums: number[] = [1.0, 2.0, 3.0];

function main() {
    let a1 = new A(1.0, 2.0, 3.0);
    let a2 = new A(...nums);
    let a3 = new A(...foo());
    arktest.assertEQ(a1.fld.length, a2.fld.length)
    arktest.assertEQ(a2.fld.length, a3.fld.length)
    for (let i = 0; i < a1.fld.length; ++i) {
        arktest.assertEQ(a1.fld[i], a2.fld[i])
        arktest.assertEQ(a2.fld[i], a3.fld[i])
    }
}
