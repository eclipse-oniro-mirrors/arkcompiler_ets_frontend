/**
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

import { B, B1 } from './recheck_import_B.ets'
import { d } from './recheck_import_D.ets'

function mySin(a: number, b: number) {
    return Math.sin(a + b);
}
let x: number = 1;


class SinHandler {
    a: number = 18
    b: number = 6
    foo() : number {
        return Math.sin(this.a / this.b);
    }
}

function main() {
    let aSin = new SinHandler();
    arktest.assertEQ(x, 1)
    console.log(mySin(aSin.foo(), x));
    let b = new B();
    let b1 = new B1();
    let bAssert = 5;
    arktest.assertEQ(b.b, bAssert, "expected: " + bAssert + " actual: " + b.b)
    console.log(b.b);
}