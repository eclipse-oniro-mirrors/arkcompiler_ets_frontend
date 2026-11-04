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
    v : T;
}

type Alias1Function<T> = (p : T) => double;
type Alias2Function<T> = (p : double) => T;
type Alias3Function<T> = (p : Int) => T[];
type Alias4Function<T> = (p : T[], q: T) => double;
type Alias5Function<T1, T2> = (p : T1[], q: T2) => double;
type Alias6Function<T1, T2, R> = (p : T1, q: T2) => R;
type Alias7Function<T> = (p : (p1 : C<T>) => double, q : double) => double;

function main() {
    let v1 : (p : double) => double = (p : double) : double => { return p; }; // (p:Double) => Double
    arktest.assertEQ(v1(1), 1)

    let v2 : Alias1Function<double> = (p : double) : double => { return p; }; // (p:Double) => Double
    arktest.assertEQ(v2(2), 2)

    let v3 : Alias2Function<double> = (p : double) : double => { return p; }; // (p:Double) => Double
    arktest.assertEQ(v3(3), 3)

    let v4 : Alias3Function<double> = (p : Int) : double[] => { return [p]; }; // (p:Double) => double[]
    arktest.assertEQ(v4(4)[0], 4)

    // let v5 : Alias4Function<double> = (p : double[], q: double) : double => { return p[0]; };  // (p: Double[], q: Double) => Double
    // arktest.assertEQ(v5([5.0], 6), 5)

    let v6 : Alias5Function<double, double> = (p : double[], q: double) : double => { return p[0]; };  // (p: double[], q: Double) => Double
    arktest.assertEQ(v6([7.0], 8), 7.0)

    let v7 : Alias6Function<double, double, double> = (p: double, q: double) : double => { return p + q; }; // (p: Double, q: Double) => Double
    arktest.assertEQ(v7(9, 10), 19)
    
    let v9 : Alias7Function<double> = (p : (p1 : C<double>) => double, q: double) : double => { return p(new C<double>(q)); }; // (p : (p1 : C<Double) => Double, Double) : Double
    arktest.assertEQ(v9((p : C<double>) : double => { return p.v;}, 20), 20);
}    
