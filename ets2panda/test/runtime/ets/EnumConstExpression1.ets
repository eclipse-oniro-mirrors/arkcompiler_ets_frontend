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

const a: string = 1 + 123123123111311 + 3 + "a a" + 111 + 221 + 3113.0;
const b: string = 1.23 + "hello" + 4;
const c: string = 0.123e5 + "world" + 10;
const d: string = 1e100 + "large float" + 100;
const e: string = 3.14 + "pi" + 25;
const f: string = 1.1 + 2.2 + "plus floats" + 10;
const g: string = 1000 + " small float" + 1;
const h: string = 3.0 + "equal float" + 5;
const i: string = 0.000000123 + "tiny float" + 9;
const j: string = 10.0 + 1.234 + " numbers together" + 2;
const k: string = 1.0 + "just one" + 0.5;
const l: string = 1e-100 + "tiny float in scientific" + 10000;
const m: string = 1e+100 + " extremely large float" + 0;
const n: string = (1.5 * 2.5) + " result of multiplication" + 3;
const o: string = 0.123456789 + " precise decimal" + 12;
const p: string = (0.1 + 0.2) + " rounding check" + 0.5;
const q: string = 1000000.12345 + " million" + 500;
const r: string = 1000 + "large number" + 99.99;
const s: string = 0.000001 + "minuscule number" + 0.00001;
const t: string = 1.2 + "concatenation" + 5000;
const u: string = 0 + "zero start" + 1.999;
const v: string = 123456789 + " number is large" + 1.2345;
const x: string = 1.7976931348623157e308 + " max float" + 100;
const y: string = -1.7976931348623157e308 + " min float" + 100;
const z: string = 2.2250738585072014e-308 + " smallest positive float" + 0.0001;

enum Color {
    A = a,
    B = b,
    C = c,
    D = d,
    E = e,
    F = f,
    G = g,
    H = h,
    I = i,
    J = j,
    K = k,
    L = l,
    M = m,
    N = n,
    O = o,
    P = p,
    Q = q,
    R = r,
    S = s,
    T = t,
    U = u,
    V = v,
    X = x,
    Y = y,
    Z = z,
}

function main() {
    arktest.assertEQ(Color.A.toString(), a)
    arktest.assertEQ(Color.B.toString(), b)
    arktest.assertEQ(Color.C.toString(), c)
    arktest.assertEQ(Color.D.toString(), d)
    arktest.assertEQ(Color.E.toString(), e)
    arktest.assertEQ(Color.F.toString(), f)
    arktest.assertEQ(Color.G.toString(), g)
    arktest.assertEQ(Color.H.toString(), h)
    arktest.assertEQ(Color.I.toString(), i)
    arktest.assertEQ(Color.J.toString(), j)
    arktest.assertEQ(Color.K.toString(), k)
    arktest.assertEQ(Color.L.toString(), l)
    arktest.assertEQ(Color.M.toString(), m)
    arktest.assertEQ(Color.N.toString(), n)
    arktest.assertEQ(Color.O.toString(), o)
    arktest.assertEQ(Color.P.toString(), p)
    arktest.assertEQ(Color.Q.toString(), q)
    arktest.assertEQ(Color.R.toString(), r)
    arktest.assertEQ(Color.S.toString(), s)
    arktest.assertEQ(Color.T.toString(), t)
    arktest.assertEQ(Color.U.toString(), u)
    arktest.assertEQ(Color.V.toString(), v)
    arktest.assertEQ(Color.X.toString(), x)
    arktest.assertEQ(Color.Y.toString(), y)
    arktest.assertEQ(Color.Z.toString(), z)
}
