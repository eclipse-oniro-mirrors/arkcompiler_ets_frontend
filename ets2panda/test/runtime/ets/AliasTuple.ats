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

type Alias1Tuple<T> = [T];
type Alias2Tuple<T> = [T, Int];
type Alias3Tuple<T> = [C<T>, double];
type Alias4Tuple<T1, T2> = [T1[], T2];
type Alias5Tuple<T1, T2> = [C<T1>[], T2];
type Alias6Tuple<T1, T2> = [(p: T1) => double, T2];


function main() {
    let v1: [double] = [new Int(1)];                            //  [double]
    let v2: [double, Int] = [new Double(2), new Int(3)];        //  [Double, Int]

    let v3: Alias1Tuple<double> = [new Int(4)];                 //  [double]
    let v4: Alias2Tuple<double> = [new Double(5), new Int(6)];  //  [Double, Int]

    let v5: Alias3Tuple<double> = [new C<double>(7), 8];        //  [C<Double>, Double]
    arktest.assertEQ(v5[0].v, 7)
    arktest.assertEQ(v5[1], 8)

    let v6: Alias4Tuple<double, double> = [[new Int(9), new Int(10), new Int(11)], 12];  // [double[], Double]
    arktest.assertEQ(v6[0][0], 9)
    arktest.assertEQ(v6[0][1], 10)
    arktest.assertEQ(v6[0][2], 11)
    arktest.assertEQ(v6[1], 12)

    let v7: Alias5Tuple<double, double> = [[new C<double>(13)], 14];  // [C<Double>[], Double]
    arktest.assertEQ(v7[0][0].v, 13)
    arktest.assertEQ(v7[1], 14)

    let v9: Alias6Tuple<double, double> = [(p: double) : double => { return p;}, 16];  // [(p: Double) => Double, Double]
    arktest.assertEQ(v9[0](15), 15)
    arktest.assertEQ(v9[1], 16)
}
