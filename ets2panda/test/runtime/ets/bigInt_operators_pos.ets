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

const v0 = 123n;
const vStr = "Weens"
for (const v2 of vStr) {
    for (let v3 = 0; v3< 93; v3++) {
        const v4 = [-3.0, -921192.1546513135, -1000000.0, 0.21581641635431563, NaN, 1e-15, -1e-15, -28.514616541534];
    }
    const v1 = 1n;
    const gtTest = v1 > v0;
    const ltTest = v1 < v0;
    const gteTest = v1 >= v0;
    const lteTest = v1 <= v0;
    const eqTest = v1 == v0;
    const neqTest = v1 != v0;
    const addTest = v1 + v0;
    const subTest = v1 - v0;
    const mulTest = v1 * v0;
    const divTest = v1 / v0;
    const modTest = v1 % v0;
    const bitOrTest = v1 | v0;
    const bitAndTest = v1 & v0;
    const bitXorTest = v1 ^ v0;
    const rightTest = v1 >> v0;
    const leftTest = v1 << v0;

    arktest.assertEQ(gtTest, false);
    arktest.assertEQ(ltTest, true);
    arktest.assertEQ(gteTest, false);
    arktest.assertEQ(lteTest, true);
    arktest.assertEQ(eqTest, false);
    arktest.assertEQ(neqTest, true);

    arktest.assertEQ(addTest, 124n);
    arktest.assertEQ(subTest, -122n);
    arktest.assertEQ(mulTest, 123n);
    arktest.assertEQ(divTest, 0n);
    arktest.assertEQ(modTest, 1n );
    arktest.assertEQ(bitOrTest, 123n);
    arktest.assertEQ(bitAndTest, 1n);
    arktest.assertEQ(bitXorTest, 122n);
    arktest.assertEQ(rightTest, 0n); 
    arktest.assertEQ(leftTest, 10633823966279326983230456482242756608n); 
}
