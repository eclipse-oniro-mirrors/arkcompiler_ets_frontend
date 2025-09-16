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

class C<T> {
    constructor (p: T) { this.v = p; }
    v: T;
}

type Alias1Union<T1, T2> = T1 | T2;
type Alias2Union<T1, T2> = C<T1> | T2;
type Alias3Union<T1, T2> = C<T1>[] | T2;
type Alias4Union<T1, T2> = (p: T1) => double | T2;

function main() {
    let v1 : double | double = new Int(1);    // primitive double
    let v2 : double | string = 2;             // boxed Double|String

    let v3 : Alias1Union<double, double> = new Int(3);  // primitive double
    arktest.assertEQ(v3, 3)                           
    let v4 : Alias1Union<double, string> = 4;          // boxed Double|String
    arktest.assertEQ(v4, 4)

    let v5 : Alias2Union<double, string> = new C<double>(5);  // C<Double>|String
    let v6 : C<double> = v5 as C<double>;
    arktest.assertEQ(v6.v, 5)

    let v7 : Alias3Union<double, string> = [new C<double>(6)];   // C<Double>[]|String
    let v9 : C<double>[] = v7 as C<double>[];
    arktest.assertEQ(v9[0].v, 6)

    let v10 : Alias4Union<double, string> = (p : double) : double => { return p; };  // (p: Double)=>Double |String
    let v11 : (p : double) => double = v10 as (p : double) => double;
    arktest.assertEQ(v11(7), 7)
}
