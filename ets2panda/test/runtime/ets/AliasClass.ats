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

class D<T1, T2> {
    constructor (p: T1, q: T2) { this.v = p; this.w = q; }
    v : T1;
    w : T2;
}

type AliasPrimitive<T> = T;
type AliasAlias<T> = AliasPrimitive<T>;

type Alias1Class<T> = C<T>;
type Alias2Class<T> = C<T[]>;
type Alias3Class<T1, T2> = D<T1, T2>;
type Alias4Class<T1, T2> = D<T2, T1>;
type Alias5Class<T1, T2> = D<Alias1Class<T1>, AliasAlias<T2>[]>

function main() {
    let v1 : C<double> = new C<double>(1);                     // C<Double>
    arktest.assertEQ(v1.v, 1)

    let v2 : Alias1Class<double> = new C<double>(2);           // C<Double>
    arktest.assertEQ(v2.v, 2)

    let v3 : Alias2Class<double> = new C<double[]>([3.0]);     // C<double[]>
    arktest.assertEQ(v3.v[0], 3)

    let v4: Alias3Class<double, int> = new D<double, int>(4.0, 5); // D<Double, Int>
    arktest.assertEQ(v4.v, 4.0)
    arktest.assertEQ(v4.w, 5)

    let v5: Alias4Class<double, int> = new D<int, double>(6, 7.0); // D<Int, Double>
    arktest.assertEQ(v5.v, 6)
    arktest.assertEQ(v5.w, 7.0)

    let v6: Alias5Class<double, double> = new D<C<double>, double[]>(new C<double>(8), [9.0]);  // D<C<Double>, double[]>
    arktest.assertEQ(v6.v.v, 8)
    arktest.assertEQ(v6.w[0], 9.0)
}
