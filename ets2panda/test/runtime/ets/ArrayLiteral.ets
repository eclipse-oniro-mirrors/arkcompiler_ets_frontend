/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function main(): void {
    let a: byte = 2;
    let b: short = 20000;
    let c: int = 2000000;
    let d: long = 200000000000;
    let e: float = 2.2f;
    let f: double = 2.2222222222;
    let g: double[] = [a, b, c, d, e, f];
    arktest.assertEQ(g[0], 2)
    arktest.assertEQ(g[1], 20000)
    arktest.assertEQ(g[2], 2000000)
    arktest.assertEQ(g[3], 200000000000)
    arktest.assertEQ(g[4], (2.2f))
    arktest.assertEQ(g[5], 2.2222222222)

    const h: byte = 2;
    const i: short = 2;
    const j: int = 2;
    const k: long = 2;
    const l: float = 2.0f;
    const m: double = 2.0;
    const n: byte[] = [h, Short.toByte(i), j, Long.toByte(k), Float.toByte(l), Double.toByte(m)];
    arktest.assertEQ(n[0], 2)
    arktest.assertEQ(n[1], 2)
    arktest.assertEQ(n[2], 2)
    arktest.assertEQ(n[3], 2)
    arktest.assertEQ(n[4], 2)
    arktest.assertEQ(n[5], 2)

    let o: Object[] = [1, 1.1, "testStr", new Int(2), d, k];
    arktest.assertEQ(o[0] as Int, 1)
    arktest.assertEQ(o[1] as Double, 1.1)
    arktest.assertTrue((o[2] as String).equals("testStr"))
    arktest.assertEQ(o[3] as Int, 2)
    arktest.assertEQ(o[4] as Long, 200000000000)
    arktest.assertEQ(o[5] as Long, 2)

    let p: long[] = [new Int(3), new Short(2 as short), new Long(4)];
    arktest.assertEQ(p[0], 3)
    arktest.assertEQ(p[1], 2)
    arktest.assertEQ(p[2], 4)
}
